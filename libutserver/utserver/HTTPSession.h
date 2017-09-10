//
// Created by saman on 08/08/17.
//

#ifndef NEWWEBSERVER_HTTPSESSION_H
#define NEWWEBSERVER_HTTPSESSION_H
#include <cstdint>
#include <cstddef>
#include <unistd.h>
#include "base.h"
namespace utserver {
class HTTPServer;
class HTTPResponse;
class HTTPRequest;
class HTTPSession {
    friend class HTTPResponse;
    friend class HTTPRequest;
 private:
    HTTPServer& server;
 public:
    HTTPSession(HTTPServer& s) : server(s) {};
    void serve();
    const HTTPResponse buildResponse(HTTPRequest& request);

    // Virtual method table look up should not add significant overhead when compared to system calls issued here
    virtual ssize_t recv(void *buf, size_t len, int flags){};
    virtual ssize_t send(const void *buf, size_t len, int flags){};
    virtual int getFD(){};
    virtual void yield(){};

    ~HTTPSession() {}
};


}

#endif //NEWWEBSERVER_HTTPSESSION_H
