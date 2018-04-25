#include "fiberServer.h"

using namespace utserver;

#define PORT 8800
std::vector<int> libfibreserver::FibreServer::servers;

const HTTPResponse helloWorld(const HTTPRequest& request){
    const HTTPResponse response(1, 200, "OK", "text/plain", "Hello, World!");
    return response;
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("USAGE: %s NUMBER_OF_THREADS CPU_BASE\n", argv[0]);
        return 0;
    }
    //set total number of worker threads
    size_t thread_count = atoi(argv[1]);
    int cpubase = atoi(argv[2]);

    libfibreserver::FibreServer server("u", PORT, thread_count, cpubase);
    server.registerRoute("/plaintext", helloWorld);
    server.start();

  return 0;

}