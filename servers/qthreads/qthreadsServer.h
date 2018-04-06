//
// Created by saman on 10/09/17.
//

#ifndef NEWWEBSERVER_QTHREADSSERVER_H
#define NEWWEBSERVER_QTHREADSSERVER_H
#include "qthread/qthread.h"
#include "qthread/qt_syscalls.h"
#include <mutex> // call_once
#include "utserver.h"

namespace qthreadsserver {
// to pass to handle_connection
struct ServerAndFd {
    utserver::HTTPServer* hserver;
    int          connfd;
    ServerAndFd(utserver::HTTPServer* hs, int fd): hserver(hs), connfd(fd){};
};
const size_t MAXIMUM_THREADS_PER_SHEPPARD = 8;
// uThreads HTTP Session
class QTHTTPSession : public utserver::HTTPSession {
 private:
    int connection;
 public:
    QTHTTPSession(utserver::HTTPServer &s, int c) : HTTPSession(s), connection(c) {};

    ssize_t recv(void *buf, size_t len, int flags) {
        return qt_read(connection, buf, len); //, flags);
    };

    ssize_t send(const void *buf, size_t len, int flags) {
        return qt_write(connection, buf, len);//, flags);
    };

    int getFD() {
        return connection;
    };

    void yield() {
        // qthread_yield();
    };

    // Function to handle connections for each http session
    static aligned_t handle_connection(void *arg) {
        ServerAndFd* sfd = (ServerAndFd*) arg;
        QTHTTPSession session(*sfd->hserver, sfd->connfd);
        session.serve();
        close(sfd->connfd); // there is no qt_close!?
        delete sfd;
    }
};
class QTServer : public utserver::HTTPServer {
 protected:
    // number of sheppards
    size_t sheppard_count;

    int serverConnection; // Server Connection

    static std::vector<int> servers;
    std::once_flag signalRegisterFlag;

    static void intHandler(int signal) {
        for (auto it = QTServer::servers.begin(); it != QTServer::servers.end(); ++it) {
            close(*it);
            QTServer::servers.erase(it);
        }
        exit(1);
    };

    static aligned_t timer(void* vhttpserver) {
        HTTPServer* httpserver = (HTTPServer*)vhttpserver;
        int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (fd == -1)
        {
            std::cerr << "Cannot create timer fd!" << std::endl;
            exit(0);
        }

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
            n = qt_read(fd, &expirations, sizeof expirations);
            if (n == sizeof expirations) {
                HTTPServer::setDate(httpserver);
            } // TODO(saman): else throw an error
        }

    }

 public:
    QTServer(const std::string name, int p, int tc) : HTTPServer(name, p, tc) {

        sheppard_count = (thread_count / (MAXIMUM_THREADS_PER_SHEPPARD + 1)) + 1;

        setenv("QTHREAD_STACK_SIZE", "65536", 1);

        //Create qt_workers
        qthread_init(thread_count);

       // handle SIGINT to close the main socket
        std::call_once(signalRegisterFlag, signal, SIGINT, QTServer::intHandler);

        // Start the timer to set the server date
        qthread_spawn( QTServer::timer, (void*)this, 0, nullptr, 0, nullptr, NO_SHEPHERD, 0);

    }

    ~QTServer() {
        close(serverConnection);
        qthread_finalize();
    }

    void start() {
        serverStarted.store(true);
        struct sockaddr_in serv_addr; //structure containing an internet address
        bzero((char*) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        serverConnection = socket(AF_INET, SOCK_STREAM, 0);

        // set socket as nonblocking
        int on = 1;
        setsockopt(serverConnection, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(int));

        // bind to server address
        if (bind(serverConnection, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            LOG_ERROR("Error on binding");
            exit(1);
        };
        // query and report addressing info
        socklen_t addrlen = sizeof(serv_addr);
        getsockname(serverConnection, (sockaddr*)&serv_addr, &addrlen);
        QTServer::servers.push_back(serverConnection);
        listen(serverConnection, 65535);





        while (true) {
                int ccon = qt_accept(serverConnection, (struct sockaddr *) nullptr, nullptr);
            ServerAndFd* sfd = new ServerAndFd(this, ccon);
            qthread_spawn( QTHTTPSession::handle_connection, (void *)sfd, 0,
                                        nullptr, 0, nullptr, NO_SHEPHERD, 0);
        }

        close(serverConnection);
        qthread_finalize();
    };
};

};
#endif //NEWWEBSERVER_QTHREADSERVER_H
