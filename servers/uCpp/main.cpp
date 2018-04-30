#include "uCPPServer.h"

using namespace utserver;

#define PORT 8800

const HTTPResponse helloWorld(const HTTPRequest& request){
    const HTTPResponse response(1, 200, "OK", "text/plain", "Hello, World!");
    return response;
}

void uMain::main() {

    if (argc < 2) {
        printf("USAGE: %s NUMBER_OF_THREADS CPU_BASE\n", argv[0]);
        return;
    }
    //set total number of worker threads
    size_t thread_count = atoi(argv[1]);
    int cpubase = atoi(argv[2]);

    ucppserver::UCPPServer server("u", PORT, thread_count, cpubase);

    server.registerRoute("/plaintext", helloWorld);
    server.start();

    uRetCode = 0;
}