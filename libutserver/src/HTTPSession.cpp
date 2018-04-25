//
// Created by saman on 08/08/17.
//

#include "HTTPSession.h"
#include "HTTPServer.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"


const utserver::HTTPResponse utserver::HTTPSession::buildResponse(HTTPRequest &request) {
    utserver::HTTPServer::HTTPRouteFunc func = (this->server.route(string_buffer(request.path, request.path_len)));
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
    HTTPRequest request;
    HTTPInputStream httpis(*this);
    HTTPOutputStream httpos(*this);
    while (httpis.read(request)) {
        switch (request.parse()) {
            case HTTPRequest::ParserResult::Pipelined:
                buildResponse(request).buildResponse(httpos);
                break;
            case HTTPRequest::ParserResult::Complete: {
                const HTTPResponse resp = buildResponse(request);
                resp.buildResponse(httpos);
                httpos.writeBuffer();
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
        // if connection close is sent, close the connection
        for(int i = 0 ; i < request.num_headers; ++i){
            auto header = request.headers[i];
            if(strncasecmp(header.name, "connection", header.name_len) == 0)
                if(strncasecmp(header.value, "close", header.value_len) == 0) {
                    return;
                }
        }

    }
}