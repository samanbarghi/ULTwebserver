//
// Created by saman on 10/09/17.
//

#ifndef NEWWEBSERVER_PTHREADSSERVER_H
#define NEWWEBSERVER_PTHREADSSERVER_H
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cassert>  // assert
#include <cmath>    // ceil
#include "utserver.h"

#define AUTO_SPAWN

static const int PTHREADS = 60000;

namespace pthreadsserver {
const int defaultStackSize = 16 * 1024; // 16KB

static void *handle_connection(void *arg);

// handle errors from pthread functions
void handle_error(const std::string&& msg){
        LOG_ERROR(msg.c_str());
        exit(1);
}

// set pthread attribute for all threads
void setPthreadAttr(pthread_attr_t& attr){
    int s = pthread_attr_init(&attr);
    if (s != 0) handle_error("pthread_attr_init");

    s = pthread_attr_setstacksize(&attr, defaultStackSize);
    if (s != 0) handle_error("pthread_attr_setstacksize");
}

class Lock{
 public:
    pthread_mutex_t mutex;
    Lock() {
        if (pthread_mutex_init(&mutex, nullptr) != 0) handle_error("pthread_mutex_init");
    }
    void lock(){
       pthread_mutex_lock(&mutex);
    }
    int trylock(){
        return pthread_mutex_trylock(&mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&mutex);
    }
    ~Lock(){
        pthread_mutex_destroy(&mutex);
    }
};
class CV{
 private:
    pthread_cond_t cv;
    Lock& lock;
 public:
    CV(Lock& lock): lock(lock){
        if(pthread_cond_init(&cv, nullptr)) handle_error("pthread_cond_init");
    }
    void signal(){
        pthread_cond_signal(&cv);
    }
    void wait(){
        pthread_cond_wait(&cv, &lock.mutex);
    }
    void broadcast(){
        pthread_cond_broadcast(&cv);
    }
    ~CV(){
        pthread_cond_destroy(&cv);
    }
};

class PthreadPool : public utserver::ThreadPool<Lock, CV> {
 private:

    void create_thread(void* arg){
        pthread_attr_t attr;
        setPthreadAttr(attr);
        pthread_t thread;
        int s = pthread_create(&thread, &attr, utserver::ThreadPool<Lock, CV>::run, arg);
        pthread_detach(thread);
        if( pthread_attr_destroy(&attr) != 0) handle_error("pthread_attr_destroy");
    }
 public:
    PthreadPool(){};
    PthreadPool(size_t init_size){
        for(size_t i =0 ; i < init_size; ++i){
            //create_thread();
        }
    };

    ~PthreadPool(){
    }
};

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

};
class PthreadServer : public utserver::HTTPServer {
 protected:
	std::vector<pthread_t> pthreads;
	// pthread that runs the timer
	pthread_t timer_pthread;

    // determines the number of acceptor threads per core
    static const int core_per_acceptor = 2;

    static void intHandler(int signal) {
        exit(0);
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

    static void* acceptor(void* arg){
        PthreadServer* pserver = (PthreadServer*) arg;
        PthreadPool pool(utserver::ThreadPool<Lock,CV>::defaultPoolSize);
        while (true) {
            int conn_fd = ::accept4(pserver->server_conn_fd, (struct sockaddr *) nullptr, nullptr, 0);
            if(conn_fd >=0){
                pool.start(handle_connection, (void*) new ServerAndFd(pserver, conn_fd));
            }else{
                assert(errno == ECONNRESET);
                std::cout << "ECONNRESET" << std::endl;
            }
        }
        pool.stop();
    }
 public:
    int server_conn_fd;
	// threadcount here is used to determine the number of cores available
    PthreadServer(const std::string name, int p, int threadcount, int cpubase) : HTTPServer(name, p, threadcount){
        // handle SIGINT to close the main socket
        struct sigaction sa;
        sa.sa_handler = &PthreadServer::intHandler;
        // Restart the system call, if at all possible
        sa.sa_flags = SA_RESTART;
        // Block every signal during the handler
        sigfillset(&sa.sa_mask);
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            perror("Error: cannot handle SIGINT");
        }

        // Start the timer to set the server date
        pthread_attr_t attr;
		setPthreadAttr(attr);
		int s = pthread_create(&timer_pthread, &attr, PthreadServer::timer, (void *) this);
		if(s != 0) handle_error("pthread_create");
        if( pthread_attr_destroy(&attr) != 0) handle_error("pthread_attr_destroy");
    }

    ~PthreadServer() {
		::close(server_conn_fd);
    }

    void start() {
        serverStarted.store(true);
        server_conn_fd = ::socket(AF_INET, SOCK_STREAM, 0);
         int on = 1;
         setsockopt(server_conn_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(int));

        if (bind(server_conn_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            handle_error("Error on binding");
        };
        ::listen(server_conn_fd, 65535);

#ifdef AUTO_SPAWN
        // thread_count represents the number of cores
        static const int ac_count = ceil((float)thread_count/(float)core_per_acceptor);
        pthread_t ac_threads[ac_count];
        for (int i = 0; i < ac_count; ++i){
            pthread_attr_t attr;
            setPthreadAttr(attr);
            int s = pthread_create(&ac_threads[i], &attr, PthreadServer::acceptor, (void *) this);
            if(s != 0) handle_error("pthread_create");
            if( pthread_attr_destroy(&attr) != 0) handle_error("pthread_attr_destroy");
        }
        // wait for pthreads to join
        for(auto pt : ac_threads){
            (void) pthread_join(pt, nullptr);
        }
#else
        pthread_t* pthreads = new pthread_t(PTHREADS);
        pthread_attr_t attr;
        setPthreadAttr(attr);
        for(size_t i = 0; i < PTHREADS; i++){
            int s = pthread_create(&pthreads[i], &attr, handle_connection, (void *) this);
            if(s != 0) handle_error("pthread_create");
        }
        if( pthread_attr_destroy(&attr) != 0) handle_error("pthread_attr_destroy");
        // wait for pthreads to join
        for(size_t i = 0; i < PTHREADS; i++){
            (void) pthread_join(pthreads[i], nullptr);
        }
#endif
    };
};

#ifdef AUTO_SPAWN
// Function to handle connections for each http session
static void *handle_connection(void *arg) {
    ServerAndFd* snf = (ServerAndFd*) arg;
    PthreadHTTPSession session(*(utserver::HTTPServer*)snf->pserver, snf->connfd);
    session.serve();
    ::close(snf->connfd);
    delete(snf);
}
#else
static void *handle_connection(void *arg) {
    PthreadServer* pserver = (PthreadServer*) arg;
    struct sockaddr_in serv_addr;
    while(true){
        socklen_t addrlen = sizeof(serv_addr);
        int fd = accept(pserver->server_conn_fd, (sockaddr*)&serv_addr, &addrlen);
        if (fd >= 0){
            PthreadHTTPSession session(*(utserver::HTTPServer*)pserver, fd);
            session.serve();
            ::close(fd);
        }else {
            assert(errno == ECONNRESET);
            std::cout << "ECONNRESET" << std::endl;
        }
    }
}
#endif
};
#endif //NEWWEBSERVER_PTHREADSERVER_H
