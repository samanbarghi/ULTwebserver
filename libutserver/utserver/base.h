//
// Created by saman on 31/07/17.
//

#ifndef NEWWEBSERVER_BASE_H
#define NEWWEBSERVER_BASE_H

#define INPUT_BUFFER_LENGTH 1048 //1 KB
#define OUTPUT_BUFFER_LENGTH 2048 //2 KB
#define MAXIMUM_THREADS_PER_CLUSTER 8

#define LOG(msg) puts(msg);
#define LOGF(fmt, params...) printf(fmt "\n", params);
#define LOG_ERROR(msg) perror(msg);

#define likely(x)   __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)

#define HTTP_HEADERS_MAX 32

/* HTTP responses*/
#define RESPONSE_METHOD_NOT_ALLOWED "HTTP/1.1 405 Method Not Allowed\r\n"
#define RESPONSE_NOT_FOUND  "<html>\n" \
                            " <body>\n" \
                            "  <h1>Not Found</h1>\n" \
                            "  <p>The requested URL was not found on this server.</p>\n" \
                            " </body>\n" \
                            "</html>\n"


#endif //NEWWEBSERVER_BASE_H
