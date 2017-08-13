//
// Created by saman on 09/08/17.
//

#ifndef NEWWEBSERVER_HTTPSERVER_H
#define NEWWEBSERVER_HTTPSERVER_H

#include <netinet/in.h>
#include <strings.h>
#include <unordered_map>
#include <vector>
#include <signal.h>
#include "uThreads/uThreads.h"
#include "HTTPSession.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"

namespace utserver {
class HTTPServer {
    size_t thread_count;
    size_t cluster_count;
    int port;

    uThreads::runtime::Cluster &defaultCluster;

    uThreads::io::Connection *serverConnection; // Server Connection
    struct sockaddr_in serv_addr; // structure containing an internet address

    static std::vector<uThreads::io::Connection*> servers;
    std::once_flag signalRegisterFlag;
    // std::unordered_map<std::string, HTTPResponse& (*)(HTTPRequest&));

    static void intHandler(int signal) {
        for (auto it = HTTPServer::servers.begin(); it!=HTTPServer::servers.end(); ++it) {
            (*it)->close();
            delete (*it);
            HTTPServer::servers.erase(it);
        }
        exit(1);
    }
 public:
    HTTPServer(int p, size_t tc) : port(p), thread_count(tc), defaultCluster(uThreads::runtime::Cluster::getDefaultCluster()) {
        //Create clusters based on MAXIMUM_THREADS_PER_CLUSTER
        cluster_count = (thread_count / (MAXIMUM_THREADS_PER_CLUSTER + 1)) + 1;

        //Create kThreads, default thread is already started --> i=1
        uThreads::runtime::kThread *kThreads[thread_count - 1];
        for (size_t i = 1; i < thread_count - 1; i++)
        kThreads[i] = new uThreads::runtime::kThread(defaultCluster);


        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        //handle SIGINT to close the main socket
        std::call_once(signalRegisterFlag, signal, SIGINT, HTTPServer::intHandler);
    }

    ~HTTPServer(){
        serverConnection->close();
        //for (uThreads::runtime::kThread *kt: kThreads) delete kt;
    }

    void start() {
            try {
            serverConnection = new uThreads::io::Connection(AF_INET, SOCK_STREAM, 0);

            if (serverConnection->bind((struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                LOG_ERROR("Error on binding");
                exit(1);
            };

            HTTPServer::servers.push_back(serverConnection);
            serverConnection->listen(65535);

            while (true) {
                uThreads::io::Connection *clientConnection = serverConnection->accept((struct sockaddr *) nullptr, nullptr);
                uThreads::runtime::uThread::create()->start(defaultCluster, (void *) HTTPSession::handle_connection, (void*) this, (void*)clientConnection);
            }
        } catch (std::system_error &error) {
            std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
        }
    }
};

} // namespace utserver
#endif //NEWWEBSERVER_HTTPSERVER_H
