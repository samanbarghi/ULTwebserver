#include "libutserver/utserver.h"

using namespace uThreads::io;
using namespace uThreads::runtime;
using namespace utserver;

const HTTPResponse helloWorld(const HTTPRequest& request){
    const HTTPResponse response(1, 200, "OK", "text/plain", "Hello, World!");

    return response;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("USAGE: %s NUMBER_OF_THREADS\n", argv[0]);
        return 0;
    }
    //set total number of worker threads
    size_t thread_count = atoi(argv[1]);

    utserver::UTServer server("u", PORT, thread_count);
    server.registerRoute("/plaintext", helloWorld);
    server.start();

  return 0;

}