//
// Created by saman on 12/08/17.
//

#ifndef NEWWEBSERVER_HTTPREQUEST_H
#define NEWWEBSERVER_HTTPREQUEST_H
#include "base.h"
#include "strings.h"
#include <experimental/string_view>
#include "../../picohttpparser/picohttpparser.h"

namespace utserver {
class HTTPSession;
class HTTPInputStream;

class HTTPRequest {
    friend class HTTPSession;
    friend class HTTPInputStream;
    char buffer[INPUT_BUFFER_LENGTH]; //read buffer from the socket
    ssize_t nrecvd; //return value for for the read()

    // parser related variables
    const char *method, *path;
    int pret, minor_version;
    size_t buflen, prevbuflen, num_parsed, method_len, path_len, num_headers;
    struct phr_header headers[HTTP_HEADERS_MAX];

    HTTPRequest(): buflen(0), prevbuflen(0), num_parsed(0), method_len(0), path_len(0), num_headers(HTTP_HEADERS_MAX){
        bzero(buffer, INPUT_BUFFER_LENGTH);
    };

    enum ParserResult{
            Complete,
            Pipelined,
            Partial,
            Overflow,
            Error
    };

    void reset(){
        buflen = prevbuflen = 0;
        num_parsed = 0;
    }

    HTTPRequest::ParserResult parse() {
        pret = phr_parse_request(buffer + num_parsed, buflen - num_parsed, &method, &method_len, &path, &path_len,
                                 &minor_version, headers, &num_headers, prevbuflen);

        if (likely(pret > 0)) { // parse successful
            num_parsed += pret;
            if (num_parsed == buflen) {
                //reset buffer numbers
                return ParserResult::Complete;
            } else {
                // prevbuf should be set to 0 to avoid assuming partial parsing
                prevbuflen = 0;
                return ParserResult::Pipelined;
            }
        } else if (pret == -1) { // error
            LOG_ERROR("Error in Parsing the request!");
            return ParserResult::Error;
        } //else pret == -2, partial -> keep reading

        if (buflen == INPUT_BUFFER_LENGTH) {
            LOG_ERROR("Request is too long!");
            return ParserResult::Overflow;
        }
        return ParserResult::Partial;
    }
 public:
    int const getMinorVersion() const{
        return minor_version;
    }

    const std::experimental::string_view getMethod() const {
        return std::experimental::string_view(method, method_len);
    }

    const std::experimental::string_view getPath() const{
        return std::experimental::string_view(path, path_len);
    }

    // TODO: wrap headers in string_view and return them

};
} // namespace utserver


#endif //NEWWEBSERVER_HTTPREQUEST_H
