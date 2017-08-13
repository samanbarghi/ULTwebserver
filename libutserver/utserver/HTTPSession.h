//
// Created by saman on 08/08/17.
//

#ifndef NEWWEBSERVER_HTTPSESSION_H
#define NEWWEBSERVER_HTTPSESSION_H

#include "uThreads/uThreads.h"
#include "base.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"

namespace utserver {
class HTTPServer;
class HTTPSession {
 private:
    uThreads::io::Connection *connection;
    HTTPServer& server;
 public:
    HTTPSession(HTTPServer& s, uThreads::io::Connection* c) : connection(c), server(s) {};

    static void *handle_connection(void *arg1, void *arg2) {
        HTTPServer *server = (HTTPServer*) arg1;
        uThreads::io::Connection *ccon = (uThreads::io::Connection*)arg2;
        HTTPSession session(*server, ccon);
        session.serve();
        ccon->close();
        delete ccon;
    }

    void serve() {
        HTTPRequest request(*connection);
        HTTPResponse response(*connection);
        while(request.read()){
            switch(request.parse()){
                case HTTPRequest::ParserResult::Pipelined:
                    response.concat();
                    break;
                case HTTPRequest::ParserResult::Complete:
                    response.concat();
                    response.write();
                    request.reset();
                    break;
                case HTTPRequest::ParserResult::Error:
                case HTTPRequest::ParserResult::Overflow:
                    request.reset();
                case HTTPRequest::ParserResult::Partial:
                    // keep reading
                    break;
            };
        }
    }
    ~HTTPSession() {}
};
}

#endif //NEWWEBSERVER_HTTPSESSION_H
