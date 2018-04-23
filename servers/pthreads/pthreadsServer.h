//
// Created by saman on 10/09/17.
//

#ifndef NEWWEBSERVER_PTHREADSSERVER_H
#define NEWWEBSERVER_PTHREADSSERVER_H
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <mutex>
#include "utserver.h"

namespace pthreadsserver {
class PthreadServer;
// to pass to handle_connection
struct ServerAndFd {
    PthreadServer*  pserver;
    int             connfd;
    ServerAndFd(PthreadServer* ps, int fd): pserver(ps), connfd(fd){};
};

// Pthreads HTTP Session
class PthreadHTTPSession : public utserver::HTTPSession {
 private:
     int conn_fd;
 public:
    PthreadHTTPSession(utserver::HTTPServer &s, int conn_fd) : HTTPSession(s), conn_fd(conn_fd) {};

    ssize_t recv(void *buf, size_t len, int flags) {
        // blocking recv
        return ::recv(conn_fd, buf, len, flags);
    };

    ssize_t send(const void *buf, size_t len, int flags) {
        // blocking send
        return ::send(conn_fd, buf, len, flags);
    };

    int getFD() {
        return conn_fd;
    };

    void yield() {
        pthread_yield();
    };

    // Function to handle connections for each http session
    static void *handle_connection(void *arg) {
        ServerAndFd* snf = (ServerAndFd*) arg;
        PthreadHTTPSession session(*(utserver::HTTPServer*)snf->pserver, snf->connfd);
        session.serve();
        ::close(snf->connfd);
        delete(snf);
    }
};
class PthreadServer : public utserver::HTTPServer {
 protected:
    int server_conn_fd;
    std::once_flag signalRegisterFlag;
    const int defaultStackSize = 16 * 1024; // 16KB
	std::vector<pthread_t> pthreads;
	// pthread that runs the timer
	pthread_t timer_pthread;

    static std::vector<int> servers;

    static void intHandler(int signal) {
        for (auto it = PthreadServer::servers.begin(); it != PthreadServer::servers.end(); ++it) {
            ::close(*it);
            PthreadServer::servers.erase(it);
        }
        exit(1);
    };

    static void* timer(void* vhttpserver) {
        HTTPServer* httpserver = (HTTPServer*)vhttpserver;
        int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
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
            n = read(fd, &expirations, sizeof expirations);
            if (n == sizeof expirations) {
                HTTPServer::setDate(httpserver);
            } // TODO(saman): else throw an error
        }

    }

	void handle_error(const std::string&& msg){
		   	LOG_ERROR(msg.c_str());
			exit(1);
	}

	void setPthreadAttr(pthread_attr_t& attr){
		int s = pthread_attr_init(&attr);
		if (s != 0) handle_error("pthread_attr_init");

		s = pthread_attr_setstacksize(&attr, defaultStackSize);
		if (s != 0) handle_error("pthread_attr_setstacksize");
	}
 public:
	// threadcount here is used to determine the number of cores available
    PthreadServer(const std::string name, int p, int threadcount, int cpubase) : HTTPServer(name, p, threadcount){
        // handle SIGINT to close the main socket
        std::call_once(signalRegisterFlag, signal, SIGINT, PthreadServer::intHandler);

        // Start the timer to set the server date
        pthread_attr_t attr;
		setPthreadAttr(attr);
		int s = pthread_create(&timer_pthread, &attr, PthreadServer::timer, (void *) this);
		if(s != 0) handle_error("pthread_create");
        if( pthread_attr_destroy(&attr) != 0) handle_error("pthread_attr_destroy");
    }

    ~PthreadServer() {
		::close(server_conn_fd);
        //for (uThreads::runtime::kThread *kt: kThreads) delete kt;
    }

    void start() {
        serverStarted.store(true);
        server_conn_fd = ::socket(AF_INET, SOCK_STREAM, 0);

        if (bind(server_conn_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            handle_error("Error on binding");
        };
        servers.push_back(server_conn_fd);
        ::listen(server_conn_fd, 65535);

        while (true) {
            int conn_fd = ::accept4(server_conn_fd, (struct sockaddr *) nullptr, nullptr, 0);
            pthread_attr_t attr;
            setPthreadAttr(attr);
            pthread_t thread;
            int s = pthread_create(&thread, &attr, PthreadHTTPSession::handle_connection, (void*) new ServerAndFd(this, conn_fd));
            pthread_detach(thread);
            if( pthread_attr_destroy(&attr) != 0) handle_error("pthread_attr_destroy");
        }
    };
};

};
#endif //NEWWEBSERVER_PTHREADSERVER_H
