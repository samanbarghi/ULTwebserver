#include "libutserver/utserver.h"
#include <vector>

std::vector<uThreads::io::Connection*> utserver::HTTPServer::servers;


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



using namespace uThreads::io;
using namespace uThreads::runtime;

void intHandler(int sig) {

}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("USAGE: %s NUMBER_OF_THREADS\n", argv[0]);
        return 0;
    }
    //set total number of worker threads
    size_t thread_count = atoi(argv[1]);

    utserver::HTTPServer server(PORT, thread_count);
    server.start();

  return 0;

}