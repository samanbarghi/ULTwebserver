//
// Created by saman on 09/08/17.
//

#ifndef NEWWEBSERVER_HTTPSERVER_H
#define NEWWEBSERVER_HTTPSERVER_H

#include <netinet/in.h>
#include <strings.h>
#include <experimental/string_view>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>
#include <signal.h>
#include <sys/timerfd.h>
#include "uThreads/uThreads.h"
#include "HTTPSession.h"

namespace utserver {
class HTTPResponse;
class HTTPRequest;
class HTTPServer {
 public:
    typedef const HTTPResponse (*HTTPRouteFunc)(const HTTPRequest&);
    friend class HTTPSession;
    friend class HTTPResponse;

    // server date
    char                         date[32];

    static void* timer(void* vhttpserver){
        HTTPServer* httpserver = (HTTPServer*)vhttpserver;
        int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (fd == -1)
        {
            std::cerr << "Cannot create timer fd!" << std::endl;
            exit(0);
        }
        uThreads::io::Connection connection(fd);

        int e;
        const struct itimerspec its =  {
        .it_interval = {.tv_sec = 1000000000 / 1000000000, .tv_nsec =  1000000000 % 1000000000},
        .it_value = {.tv_sec = 1000000000 / 1000000000, .tv_nsec = 1000000000 % 1000000000}
        };
        e = timerfd_settime(fd, 0, &its, NULL);
        if (e == -1){
            std::cerr << "Cannot set timerfd" << std::endl;
            exit(0);
        }
        uint64_t expirations;
        ssize_t n;
        for(;;){
            n = connection.read(&expirations, sizeof expirations);
            if (n == sizeof expirations) {
                HTTPServer::setDate(httpserver);
            } // TODO(saman): else throw an error
        }

    }

    static void setDate(HTTPServer* server)
    {
        static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        time_t t;
        struct tm tm;

        (void) time(&t);
        (void) gmtime_r(&t, &tm);
        (void) strftime(server->date, sizeof server->date, "---, %d --- %Y %H:%M:%S GMT", &tm);
        std::memcpy(server->date, days[tm.tm_wday], 3);
        std::memcpy(server->date + 8, months[tm.tm_mon], 3);
    }

 protected:
    size_t thread_count;
    int port;
    const std::string name;

    struct sockaddr_in serv_addr; // structure containing an internet address

    std::atomic_bool serverStarted;
    std::unordered_map<std::experimental::string_view, HTTPRouteFunc, std::hash<std::experimental::string_view>> routes;
    std::vector<std::string> paths; // holds the path strings for routes

    const HTTPRouteFunc route(const std::experimental::string_view& route){
        return routes[route];
    }
 public:
    HTTPServer(const std::string serverName, int p, size_t tc) : name(serverName), port(p), thread_count(tc), serverStarted(false) {

       bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        setDate(this);

   }

    ~HTTPServer(){
   }

    virtual void start(){};

    // A pointer to a function is passed instead of std::function or anything callable
    // this is to avoid the unnecessary overhead that these methods add.
    bool registerRoute(std::string route, HTTPRouteFunc func){
        // Route registration is only permitted before the server starts,
        // otherwise the overhead of synchronization can affect the results
        if(serverStarted.load())
            return false;
        if (std::find(paths.begin(), paths.end(), route) != paths.end()) {
            // already exists -> TODO: maybe return enum values?
            return false;
        }
        paths.push_back(route);
        auto it = &paths.back();
        std::experimental::string_view key(it->c_str(), it->size());
        return routes.insert({key, func}).second;
    }
};

class UTServer: public HTTPServer{
 protected:
    // default Cluster
    uThreads::runtime::Cluster &defaultCluster;
    // number of clusters
    size_t cluster_count;

    uThreads::io::Connection *serverConnection; // Server Connection

    static std::vector<uThreads::io::Connection*> servers;
    std::once_flag signalRegisterFlag;

    static void intHandler(int signal) {
        for (auto it = UTServer::servers.begin(); it!=UTServer::servers.end(); ++it) {
            (*it)->close();
            delete (*it);
            UTServer::servers.erase(it);
        }
        exit(1);
    };
 public:
   UTServer(const std::string name, int p, int tc) : HTTPServer(name, p, tc), defaultCluster(uThreads::runtime::Cluster::getDefaultCluster()) {

        cluster_count = (thread_count / (MAXIMUM_THREADS_PER_CLUSTER + 1)) + 1;

        //Create kThreads, default thread is already started --> i=1
        uThreads::runtime::kThread *kThreads[thread_count - 1];
        for (size_t i = 1; i < thread_count - 1; i++)
            kThreads[i] = new uThreads::runtime::kThread(defaultCluster);        //Create clusters based on MAXIMUM_THREADS_PER_CLUSTER

        // handle SIGINT to close the main socket
        std::call_once(signalRegisterFlag, signal, SIGINT, UTServer::intHandler);

        // Start the timer to set the server date
        uThreads::runtime::uThread::create()->start(defaultCluster, (void *) HTTPServer::timer, (void*) this);

   }
    ~UTServer(){
        serverConnection->close();
        //for (uThreads::runtime::kThread *kt: kThreads) delete kt;
    }

    void start() {
            serverStarted.store(true);
            try {
            serverConnection = new uThreads::io::Connection(AF_INET, SOCK_STREAM, 0);

            if (serverConnection->bind((struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                LOG_ERROR("Error on binding");
                exit(1);
            };

            UTServer::servers.push_back(serverConnection);
            serverConnection->listen(65535);

            while (true) {
                uThreads::io::Connection *clientConnection = serverConnection->accept((struct sockaddr *) nullptr, nullptr);
                uThreads::runtime::uThread::create()->start(defaultCluster, (void *) HTTPSession::handle_connection, (void*) this, (void*)clientConnection);
            }
        } catch (std::system_error &error) {
            std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
        }
    };
};
} // namespace utserver
#endif //NEWWEBSERVER_HTTPSERVER_H
