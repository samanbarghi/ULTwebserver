//
// Created by saman on 15/08/17.
//

#ifndef NEWWEBSERVER_HTTPSTREAM_H
#define NEWWEBSERVER_HTTPSTREAM_H
#include "base.h"
#include <string.h>

namespace utserver{
class HTTPSession;
class HTTPRequest;
class HTTPresponse;

class HTTPInputStream{
    friend class HTTPRequest;
    friend class HTTPSession;

    HTTPSession& session;

    HTTPInputStream(HTTPSession& s): session(s){};

    bool read(HTTPRequest& request) {

        ssize_t nrecvd; //return value for for the read()
        request.num_headers = HTTP_HEADERS_MAX;
        // Should we read again? or just parse what is left in the buffer?
        if(!request.num_parsed) {
            //Since we only accept GET, just try to read INPUT_BUFFER_LENGTH
            session.yield();
            do {
                nrecvd = session.recv(request.buffer + request.buflen , INPUT_BUFFER_LENGTH - request.buflen, 0);
                if (unlikely(nrecvd<= 0)) {
                    // connection was closed
                    if (nrecvd == 0)
                        return 0;
                    else if(errno != EINTR){
                        //if RST packet by browser, just close the connection
                        //no need to show an error.
                        if(errno != ECONNRESET){
                            LOG_ERROR("Error reading from socket");
                            printf("fd %d\n", session.getFD());
                        }
                        return false;
                    }
                    // Otherwise, 'read' was interrupted so try again
                }
                // nrecvd > 0 -> read successful
                break;
            } while (true);
            request.prevbuflen = request.buflen;
            request.buflen += nrecvd;
        }
        return true;
    }
};

class HTTPOutputStream{
    friend class HTTPResponse;
    friend class HTTPSession;
    HTTPSession& session;

    // output buffer
    char output_buffer[OUTPUT_BUFFER_LENGTH]; //write buffer to the socket
    size_t output_length;


 public:
    HTTPOutputStream(HTTPSession& httpsession): session(httpsession), output_length(0){
            bzero(output_buffer, OUTPUT_BUFFER_LENGTH);
    }
 private:
    void reset() {
        output_length = 0;
    }
    void concat(const std::string& responseStr){

        // if we are going to exceed the buffer, flush the buffer
        if(unlikely( responseStr.length() > (OUTPUT_BUFFER_LENGTH - output_length)))
            writeBuffer();
        // if response length is larger than OUTPUT_BUFFER_LENGTH write directly
        if(unlikely( responseStr.length() > OUTPUT_BUFFER_LENGTH)){
            writeSingle(responseStr);
            return;
        }
        strncpy(output_buffer + output_length, responseStr.c_str(), responseStr.length());
        output_length += responseStr.length();
    };

    bool writeSingle(const std::string& response){
        return write(response.c_str(), response.length());
    };

    bool writeBuffer(){
        // std::cout << output_buffer << std::endl;
        return write(output_buffer, output_length);
    };

    bool write(const char* vptr, size_t length) {

        size_t nleft;
        ssize_t nwritten;
        const char *ptr;

        ptr = (char *) vptr;
        nleft = length;

        while (nleft > 0) {
            if ((nwritten = session.send(ptr, nleft, 0)) <= 0) {
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
}
#endif //NEWWEBSERVER_HTTPSTREAM_H
