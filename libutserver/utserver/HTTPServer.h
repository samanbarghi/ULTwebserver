//
// Created by saman on 09/08/17.
//

#ifndef NEWWEBSERVER_HTTPSERVER_H
#define NEWWEBSERVER_HTTPSERVER_H
#include <netinet/in.h>
#include <strings.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>
#include <signal.h>
#include <sys/timerfd.h>
#include <atomic>
#include <iostream>
#include "HTTPSession.h"

namespace utserver {
class HTTPResponse;
class HTTPRequest;
class HTTPServer {
 public:
    typedef const HTTPResponse (*HTTPRouteFunc)(const HTTPRequest&);
    friend class HTTPSession;
    friend class HTTPResponse;


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

    // server date
    // Children of this class should make sure to set this using a timer
    char                         date[32];

    struct sockaddr_in serv_addr; // structure containing an internet address

    std::atomic_bool serverStarted;
    // TODO: This should change to a map for larger servers. For now
    // string_view does not work with uC++ so we change it to a vector
    std::vector<std::pair<string_buffer, HTTPRouteFunc>> routes;
    std::vector<std::string> paths; // holds the path strings for routes
    std::vector<string_buffer> pathsView; // holds the path strings for routes

    const HTTPRouteFunc route(const string_buffer route){
        for(auto p : routes)
            if(p.first == route) return p.second;
        return nullptr;
    }
 public:
    // TODO(saman): inizial size of the vectors should be received from the user? or use array instead?
    HTTPServer(const std::string serverName, int p, size_t tc) : name(serverName), port(p), thread_count(tc), serverStarted(false), paths(100), pathsView(100) {

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
        pathsView.emplace_back(it->c_str(), it->size());
        auto itv = &pathsView.back();
        routes.emplace_back(std::make_pair(*itv, func));
        return true;
    }
};

} // namespace utserver
#endif //NEWWEBSERVER_HTTPSERVER_H
