//
// Created by saman on 10/09/17.
//

#ifndef NEWWEBSERVER_UTHREADSSERVER_H
#define NEWWEBSERVER_UTHREADSSERVER_H
#include "uThreads/uThreads.h"
#include "utserver.h"

namespace uthreadsserver {
// uThreads HTTP Session
class UTHTTPSession : public utserver::HTTPSession {
 private:
    uThreads::io::Connection *connection;
 public:
    UTHTTPSession(utserver::HTTPServer &s, uThreads::io::Connection *c) : HTTPSession(s), connection(c) {};

    ssize_t recv(void *buf, size_t len, int flags) {
        return connection->recv(buf, len, flags);
    };

    ssize_t send(const void *buf, size_t len, int flags) {
        return connection->send(buf, len, flags);
    };

    int getFD() {
        return connection->getFd();
    };

    void yield() {
        uThreads::runtime::uThread::yield();
    };

    // Function to handle connections for each http session
    static void *handle_connection(void *arg1, void *arg2) {
        utserver::HTTPServer *server = (utserver::HTTPServer *) arg1;
        uThreads::io::Connection *ccon = (uThreads::io::Connection *) arg2;
        UTHTTPSession session(*server, ccon);
        session.serve();
        ccon->close();
        delete ccon;
    }
};
class UTServer : public utserver::HTTPServer {
 protected:
    // default Cluster
    uThreads::runtime::Cluster &defaultCluster;
    // number of clusters
    size_t cluster_count;

    uThreads::io::Connection *serverConnection; // Server Connection

    static std::vector<uThreads::io::Connection *> servers;
    std::once_flag signalRegisterFlag;

    static void intHandler(int signal) {
        for (auto it = UTServer::servers.begin(); it != UTServer::servers.end(); ++it) {
            (*it)->close();
            delete (*it);
            UTServer::servers.erase(it);
        }
        exit(1);
    };

    static void* timer(void* vhttpserver) {
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

 public:
    UTServer(const std::string name, int p, int tc) : HTTPServer(name, p, tc),
                                                      defaultCluster(uThreads::runtime::Cluster::getDefaultCluster()) {

        cluster_count = (thread_count / (MAXIMUM_THREADS_PER_CLUSTER + 1)) + 1;

        //Create kThreads, default thread is already started --> i=1
        uThreads::runtime::kThread *kThreads[thread_count - 1];
        for (size_t i = 1; i < thread_count - 1; i++)
            kThreads[i] = new uThreads::runtime::kThread(
                    defaultCluster);        //Create clusters based on MAXIMUM_THREADS_PER_CLUSTER

        // handle SIGINT to close the main socket
        std::call_once(signalRegisterFlag, signal, SIGINT, UTServer::intHandler);

        // Start the timer to set the server date
        uThreads::runtime::uThread::create()->start(defaultCluster, (void *) UTServer::timer, (void *) this);

    }

    ~UTServer() {
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
                uThreads::io::Connection *clientConnection = serverConnection->accept((struct sockaddr *) nullptr,
                                                                                      nullptr);
                uThreads::runtime::uThread::create()->start(defaultCluster, (void *) UTHTTPSession::handle_connection,
                                                            (void *) this, (void *) clientConnection);
            }
        } catch (std::system_error &error) {
            std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
        }
    };
};

};
#endif //NEWWEBSERVER_UTHREADSERVER_H
