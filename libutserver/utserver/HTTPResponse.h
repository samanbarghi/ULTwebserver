//
// Created by saman on 12/08/17.
//

#ifndef NEWWEBSERVER_HTTPRESPONSE_H
#define NEWWEBSERVER_HTTPRESPONSE_H
#include "base.h"
#include "uThreads/uThreads.h"
#include <string.h>

// To avoid file io, only return a simple HelloWorld!
#define RESPONSE_OK "HTTP/1.1 200 OK\r\n" \
                    "Content-Length: 13\r\n" \
                    "Content-Type: text/plain\r\n" \
                    "Date: Wed, 17 May 2017 19:48:59 GMT\r\n" \
                    "Server: u\r\n" \
                    "\r\n" \
                    "Hello, World!"
#define    HTTP_REQUEST_HEADER_END    "\n\r\n\r"


namespace utserver {
class HTTPResponse {

    uThreads::io::Connection& connection;

    char output_buffer[OUTPUT_BUFFER_LENGTH]; //write buffer to the socket
    size_t output_length;

 public:
    HTTPResponse(uThreads::io::Connection& c): connection(c), output_length(0){
        bzero(output_buffer, OUTPUT_BUFFER_LENGTH);
    }

    void reset() {
        output_length = 0;
    }
    void concat(){

        strncpy(output_buffer + output_length, RESPONSE_OK, strlen(RESPONSE_OK));
        output_length += strlen(RESPONSE_OK);
    };
    bool write() {

        size_t nleft;
        ssize_t nwritten;
        const char *ptr;

        ptr = (char *) output_buffer;
        nleft = output_length;

        while (nleft > 0) {
            if ((nwritten = connection.send(ptr, nleft, 0)) <= 0) {
                if (errno == EINTR)
                    nwritten = 0; /* If interrupted system call => call the write again */
                else
                    return false;
            }
            nleft -= nwritten;
            ptr += nwritten;
        }

        //return (n);
        reset();
        return true;
    };
};
} // namespace utserver


#endif //NEWWEBSERVER_HTTPRESPONSE_H
