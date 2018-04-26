#include "pthreadsServer.h"

using namespace utserver;
std::vector<int> pthreadsserver::PthreadServer::servers;
std::atomic_bool pthreadsserver::PthreadServer::stop(false);

#define PORT 8800

const HTTPResponse helloWorld(const HTTPRequest& request){
    const HTTPResponse response(1, 200, "OK", "text/plain", "Hello, World!");
    return response;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("USAGE: %s NUMBER_OF_CORES\n", argv[0]);
        return 0;
    }
    //set total number of worker threads
    size_t core_count = atoi(argv[1]);

    pthreadsserver::PthreadServer server("u", PORT, core_count, 0);
    server.registerRoute("/plaintext", helloWorld);
    server.start();

  return 0;

}