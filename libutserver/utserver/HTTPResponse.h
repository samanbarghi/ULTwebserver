//
// Created by saman on 12/08/17.
//

#ifndef NEWWEBSERVER_HTTPRESPONSE_H
#define NEWWEBSERVER_HTTPRESPONSE_H
#include "base.h"
#include "HTTP.h"
#include "uThreads/uThreads.h"
#include "HTTPStream.h"
#include "HTTPSession.h"
#include "HTTPServer.h"
#include <string>

#define END_OF_RESPONSE_LINE   "\r\n"
// To avoid file io, only return a simple HelloWorld!

namespace utserver {
class HTTPResponse {
    friend class HTTPSession;

 public:
    // TODO(Saman): vector seems to be a very slow solution with current implementation. Preallocate? array? is this because memory is allocated on the heap?
    std::vector<HTTPHeaders> headers;
    uint8_t _version;
    int _status;
    std::string _reason;
    std::string _contentType;

    std::string _body;

/*    HTTPResponse():_status(200), _reason("OK"), _version(1), _contentType("text/html"), _body(""), headers(){};
    HTTPResponse(std::string body):_status(200), _reason("OK"), _version(1), _contentType("text/html"), _body(body), headers(){};
    HTTPResponse(std::string contentType, std::string body):_status(200), _reason("OK"), _version(1), _contentType(contentType), _body(body), headers(){};
    HTTPResponse(uint8_t version):_status(200), _reason("OK"), _version(version), _contentType("text/html"), headers(){};
    HTTPResponse(int status, std::string reason):_status(status), _reason(reason), _version(1), _contentType("text/html"), headers(){};
    HTTPResponse(uint8_t version, int status, std::string reason):_status(status), _reason(reason), _version(version), _contentType("text/html"), headers(){};
    HTTPResponse(uint8_t version, int status, std::string reason, std::string body):_status(status), _reason(reason), _version(version), _contentType("text/html"), _body(body), headers(){};*/
    HTTPResponse(uint8_t version, int status, std::string reason, std::string contentType, std::string body):
            _status(status), _reason(reason), _version(version), _contentType(contentType), _body(body), headers(){};
 private:
    void buildStatus(HTTPOutputStream& hos) const{
        hos.concat( _version ? "HTTP/1.1 " : "HTTP/1.0 ");
        hos.concat(std::to_string(_status));
        hos.concat(" ");
        hos.concat(_reason);
        hos.concat(END_OF_RESPONSE_LINE);

    }

    void buildServerName(HTTPOutputStream& hos) const{
        hos.concat("Server: ");
        hos.concat(hos.session.server.name);
        hos.concat(END_OF_RESPONSE_LINE);
    }

    void buildDate(HTTPOutputStream& hos) const{
        hos.concat("Date: ");
        hos.concat(hos.session.server.date);
        hos.concat(END_OF_RESPONSE_LINE);
    }

    void buildContentType(HTTPOutputStream& hos) const{
        hos.concat("Content-Type: ");
        hos.concat(_contentType);
        hos.concat(END_OF_RESPONSE_LINE);
    }

    void buildHeaders(HTTPOutputStream& hos) const{
        for (auto header: headers){
            hos.concat(header._name);
            hos.concat(": ");
            hos.concat(header._value);
            hos.concat(END_OF_RESPONSE_LINE);
        }
    }

    void buildContentLength(HTTPOutputStream& hos) const{
        hos.concat("Content-Length: ");
        hos.concat(std::to_string(_body.length()));
        hos.concat(END_OF_RESPONSE_LINE);
    }

    void buildResponse(HTTPOutputStream& hos) const{
        buildStatus(hos);
        buildServerName(hos);
        buildContentType(hos);
        buildDate(hos);
        buildHeaders(hos);
        buildContentLength(hos);
        /* end of headers */
        hos.concat(END_OF_RESPONSE_LINE);
        hos.concat(_body);
    }
};
} // namespace utserver


#endif //NEWWEBSERVER_HTTPRESPONSE_H
