#include "picohttpparser/picohttpparser.h"
#include <uThreads/uThreads.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <iostream>

using uThreads::runtime::uThread;
using uThreads::runtime::kThread;
using uThreads::runtime::Cluster;
using uThreads::io::Connection;

#define PORT 8800
#define INPUT_BUFFER_LENGTH 1048 //4 KB
#define OUTPUT_BUFFER_LENGTH 2048 //4 KB
#define MAXIMUM_THREADS_PER_CLUSTER 8

/* HTTP responses*/
#define RESPONSE_METHOD_NOT_ALLOWED "HTTP/1.1 405 Method Not Allowed\r\n"
#define RESPONSE_NOT_FOUND "HTTP/1.1 404 Not Found\n" \
                            "Content-type: text/html\n" \
                            "\n" \
                            "<html>\n" \
                            " <body>\n" \
                            "  <h1>Not Found</h1>\n" \
                            "  <p>The requested URL was not found on this server.</p>\n" \
                            " </body>\n" \
                            "</html>\n"

// To avoid file io, only return a simple HelloWorld!
#define RESPONSE_OK "HTTP/1.1 200 OK\r\n" \
                    "Content-Length: 13\r\n" \
                    "Content-Type: text/plain\r\n" \
                    "Date: Wed, 17 May 2017 19:48:59 GMT\r\n" \
                    "Server: u\r\n" \
                    "\r\n" \
                    "Hello, World!"


//                    "Connection: keep-alive\r\n" \
/* Logging */
#define LOG(msg) puts(msg);
#define LOGF(fmt, params...) printf(fmt "\n", params);
#define LOG_ERROR(msg) perror(msg);
#define    HTTP_REQUEST_HEADER_END    "\n\r\n\r"

#define likely(x)   __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)

#define HTTP_HEADERS_MAX 32

Connection *sconn; //Server socket

ssize_t read_http_request(Connection &cconn, void *vptr, size_t n) {

    ssize_t nread;
    uThread::yield(); //yield before read
    do {
        nread = cconn.recv(vptr, n, 0);
        if (unlikely(nread <= 0)) {
            // connection was closed
            if (nread == 0)
                return 0;
            else if(errno != EINTR)
               return -1;
            // Otherwise, 'read' was interrupted so try again
        }
        // nread > 0 -> read successful
        return nread;
    } while (true);
}

ssize_t writen(Connection &cconn, const void *vptr, size_t n) {

    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = (char *) vptr;
    nleft = n;

    while (nleft > 0) {
        if ((nwritten = cconn.send(ptr, nleft, 0)) <= 0) {
            if (errno == EINTR)
                nwritten = 0; /* If interrupted system call => call the write again */
            else
                return (-1);
        }
        nleft -= nwritten;
        ptr += nwritten;
    }

    return (n);
}

/* handle connection after accept */
void *handle_connection(void *arg) {

    Connection *cconn = (Connection *) arg;

    char buffer[INPUT_BUFFER_LENGTH]; //read buffer from the socket
    char output_buffer[OUTPUT_BUFFER_LENGTH]; //write buffer to the socket
    bzero(buffer, INPUT_BUFFER_LENGTH);
    bzero(output_buffer, OUTPUT_BUFFER_LENGTH);

    const char *method, *path;
    int pret, minor_version;
    struct phr_header headers[HTTP_HEADERS_MAX];
    size_t buflen = 0, prevbuflen = 0, num_parsed = 0, method_len, path_len, num_headers = HTTP_HEADERS_MAX;
    ssize_t nrecvd; //return value for for the read() and write() calls.
    size_t output_length = 0;

    do {
        num_headers = HTTP_HEADERS_MAX;
        if(!num_parsed) {
            //Since we only accept GET, just try to read INPUT_BUFFER_LENGTH
            nrecvd = read_http_request(*cconn, buffer + buflen, INPUT_BUFFER_LENGTH - buflen);
            if (nrecvd == -1) {
                //if RST packet by browser, just close the connection
                //no need to show an error.
                if (errno != ECONNRESET) {
                    LOG_ERROR("Error reading from socket");
                    printf("fd %d\n", cconn->getFd());
                }
                break;
            } else if (nrecvd == 0) { //connection closed
                //std::cout << "Closed Connection" << std::endl;
                break;
            }
            prevbuflen = buflen;
            buflen += nrecvd;
//            printf("%.*s\n", nrecvd, buffer);
//            printf("=============================== \n");

        }
       pret = phr_parse_request(buffer + num_parsed, buflen - num_parsed, &method, &method_len, &path, &path_len,
                                 &minor_version, headers, &num_headers, prevbuflen);

        if (likely(pret > 0)) { // parse successful
            num_parsed += pret;
            //std::cout << buflen << ":" << num_parsed << ":" << ++num_requests << std::endl;
            //printf("method is %.*s\n", (int)method_len, method);
            strncpy(output_buffer + output_length, RESPONSE_OK, strlen(RESPONSE_OK));
            output_length += strlen(RESPONSE_OK);
            if (num_parsed == buflen) {
                //reset buffer numbers
                writen(*cconn, output_buffer, output_length);
                output_length = 0;
                buflen = prevbuflen = 0;
                num_parsed = 0;
            } else {
                // prevbuf should be set to 0 to avoid assuming partial parsing
                prevbuflen = 0;
            }
        } else if (pret == -1) { // error
            LOG_ERROR("Error in Parsing the request!");
            //reset buffer numbers
            buflen = prevbuflen = 0;
            num_parsed = 0;
            output_length = 0;
        } //else pret == -2, partial -> keep reading

        if (buflen == INPUT_BUFFER_LENGTH) {
            LOG_ERROR("Request is too long!");
        }
    } while (true);

    cconn->close();
    delete cconn;
}

void intHandler(int sig) {
    sconn->close();
    delete sconn;
    exit(1);
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("USAGE: %s NUMBER_OF_THREADS\n", argv[0]);
        return 0;
    }
    //set total number of worker threads
    size_t thread_count = atoi(argv[1]);
    //Create clusters based on MAXIMUM_THREADS_PER_CLUSTER
    size_t cluster_count = (thread_count / (MAXIMUM_THREADS_PER_CLUSTER + 1)) + 1;

    struct sockaddr_in serv_addr; //structure containing an internet address
    bzero((char *) &serv_addr, sizeof(serv_addr));

    Cluster &defaultCluster = Cluster::getDefaultCluster();
    //Create kThreads, default thread is already started --> i=1
    kThread *kThreads[thread_count - 1];
    for (size_t i = 1; i < thread_count - 1; i++)
        kThreads[i] = new kThread(defaultCluster);


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    //handle SIGINT to close the main socket
    signal(SIGINT, intHandler);

    try {
        sconn = new Connection(AF_INET, SOCK_STREAM, 0);

        if (sconn->bind((struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            LOG_ERROR("Error on binding");
            exit(1);
        };
        sconn->listen(65535);
        while (true) {
            Connection *cconn = sconn->accept((struct sockaddr *) nullptr, nullptr);
            uThread::create()->start(defaultCluster, (void *) handle_connection, (void *) cconn);
        }
        sconn->close();
    } catch (std::system_error &error) {
        std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
        sconn->close();
    }

    for (kThread *kt: kThreads) delete kt;
    return 0;

}