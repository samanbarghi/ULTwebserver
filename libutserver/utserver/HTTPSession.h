//
// Created by saman on 08/08/17.
//

#ifndef NEWWEBSERVER_HTTPSESSION_H
#define NEWWEBSERVER_HTTPSESSION_H

#include "uThreads/uThreads.h"
#include "base.h"

namespace utserver {
class HTTPServer;
class HTTPResponse;
class HTTPRequest;
class HTTPSession {
    friend class HTTPResponse;
 private:
    uThreads::io::Connection *connection;
    HTTPServer& server;
 public:
    HTTPSession(HTTPServer& s, uThreads::io::Connection* c) : connection(c), server(s) {};
    void serve();
    const HTTPResponse buildResponse(HTTPRequest& request);

    static void *handle_connection(void *arg1, void *arg2) {
        HTTPServer *server = (HTTPServer*) arg1;
        uThreads::io::Connection *ccon = (uThreads::io::Connection*)arg2;
        HTTPSession session(*server, ccon);
        session.serve();
        ccon->close();
        delete ccon;
    }

    ~HTTPSession() {}
};
}

#endif //NEWWEBSERVER_HTTPSESSION_H
