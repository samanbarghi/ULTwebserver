//
// Created by saman on 08/08/17.
//

#include "HTTPSession.h"
#include "HTTPServer.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"

std::vector<uThreads::io::Connection*> utserver::UTServer::servers;

const utserver::HTTPResponse utserver::HTTPSession::buildResponse(HTTPRequest &request) {
    utserver::HTTPServer::HTTPRouteFunc func = (this->server.route(std::experimental::string_view(request.path, request.path_len)));
    if(func){
        const HTTPResponse response = (func(request));
        return response;
    }else{
        //return not found
        const HTTPResponse response(1, 404, "Not Found", "text/html", RESPONSE_NOT_FOUND);
        return response;
    }
}
void utserver::HTTPSession::serve() {
    HTTPRequest request(*connection);
    HTTPOutputStream httpos(*this, *connection);
    while (request.read()) {
        switch (request.parse()) {
            case HTTPRequest::ParserResult::Pipelined:
                buildResponse(request).buildResponse(httpos);
                break;
            case HTTPRequest::ParserResult::Complete: {
                const HTTPResponse resp = buildResponse(request);
                // if buffer contains string, concat and write
                if (httpos.output_length) {
                    resp.buildResponse(httpos);
                    httpos.writeBuffer();
                } else { // otherwise, write directly
                    resp.buildResponse(httpos);
                    httpos.writeBuffer();
                    //httpos.writeSingle(resp);
                }
                request.reset();
                break;
            }
            case HTTPRequest::ParserResult::Error:
            case HTTPRequest::ParserResult::Overflow:
                request.reset();
            case HTTPRequest::ParserResult::Partial:
                // keep reading
                break;
        };
    }
}