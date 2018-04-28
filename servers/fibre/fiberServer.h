//
// Created by saman on 10/09/17.
//

#ifndef NEWWEBSERVER_LIBFIBRESERVER_H
#define NEWWEBSERVER_LIBFIBRESERVER_H
#include <cassert>  // assert
#include <mutex>    // call_once
#include "libfibre/fibre.h"
#include "utserver.h"


namespace libfibreserver {
class FibreServer;
// to pass to handle_connection
struct ServerAndFd {
    FibreServer* fserver;
    int          connfd;
    ServerAndFd(FibreServer* fs, int fd): fserver(fs), connfd(fd){};
};
class Lock{
public:
    Mutex<SystemLock> mutex;
    void lock(){
    	mutex.acquire();
	}
	bool trylock(){
		return mutex.tryAcquire();
	}
    void unlock(){
		mutex.release();
    }
};

class CV{
 public:
    Condition<Mutex<SystemLock>> cv;
    Lock& lock;
 public:
    CV(Lock& lock):lock(lock){}

    void signal(){
        cv.signal();
	}
    void wait(){
        cv.wait(lock.mutex);
		// libfibre does not acquire the lock after
		// cv_wait
		lock.lock();
	}
    void broadcast(){
		cv.signal<true>();
    }

};

// use the default stackPool for libfibre
class FibrePool: public utserver::ThreadPool<Lock, CV> {
 private:
	// wrapper around run, requires void
	static void run(void* arg){
		utserver::ThreadPool<Lock, CV>::run(arg);
	}
    void create_thread(void* arg){
		// create a fibre on the current cluster
        (new Fibre())->run(FibrePool::run, arg);
    }
 public:
    FibrePool(){
    };
    FibrePool(size_t init_size){
        for(size_t i =0 ; i < init_size; ++i){
            // create_thread();
        }
    };
    ~FibrePool(){
    }
};

// fibre HTTP Session
class FibreHTTPSession : public utserver::HTTPSession {
 private:
    int connection;
 public:
    // a reference to HTTPServer along with the fd this session serves
    FibreHTTPSession(utserver::HTTPServer &s, int fd) : HTTPSession(s), connection(fd) {};

    ssize_t recv(void *buf, size_t len, int flags) {
        return ::lfInput(::recv, connection, buf, len, flags);
    };

    ssize_t send(const void *buf, size_t len, int flags) {
        return ::lfOutput(::send, connection, buf, len, flags);
    };

    int getFD() {
        return connection;
    };

    void yield() {
        // Libfibre yields internally
        //Fibre::yield();
    };

    // Function to handle connections for each http session
    static void handle_connection(void *arg) {
        ServerAndFd* sfd = (ServerAndFd*)arg;
        // fd is set as the value of pointer
        FibreHTTPSession session(*(utserver::HTTPServer*)(sfd->fserver), sfd->connfd);
        session.serve();
        SYSCALL(lfClose(sfd->connfd));
        delete sfd;
    }
};

class FibreServer : public utserver::HTTPServer {
 protected:
    // number of clusters
    size_t cluster_count;
    static const int cluster_size = 8;

    std::vector<PollerCluster*> cluster;
    std::vector<SystemProcessor*> sproc;

    int serverConnection; // Server Connection

    static std::vector<int> servers;
    std::once_flag signalRegisterFlag;

    static void intHandler(int signal) {
        /*for (auto it = FibreServer::servers.begin(); it != FibreServer::servers.end(); ++it) {
            SYSCALL(lfClose(*it));
            FibreServer::servers.erase(it);
        }*/
        exit(0);
    };

    static void timer(void* vhttpserver) {
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
            n = lfInput(::read, fd, (void*)&expirations, (size_t)sizeof(expirations));
            if (n == sizeof expirations) {
                HTTPServer::setDate(httpserver);
            } // TODO(saman): else throw an error
        }
    }

 public:
    FibreServer(const std::string name, int p, int tc, int cpubase = -1) : HTTPServer(name, p, tc){

        cluster_count = ((thread_count - 1) / cluster_size) + 1; // ceiling-div
        cluster.reserve(cluster_count); // default already exists
        sproc.reserve(thread_count); // default already exists

        // for better poller to thread ratio
        int effective_cluster_size = (thread_count)/cluster_count;

        cluster.push_back(&CurrCluster());
        sproc.push_back(&CurrProcessor());
        int cluster_idx = 0;
        for (int sproc_idx = 0; sproc_idx < thread_count; sproc_idx += 1) {
            pthread_t tid;
            if (sproc_idx == 0) { // mainSP
                tid = pthread_self();
            } else {
                if (sproc_idx % effective_cluster_size == 0) {
                    cluster_idx += 1;
                    cluster.push_back(new PollerCluster);
                }
                sproc.push_back(new SystemProcessor(*cluster[cluster_idx]));
                tid = sproc[sproc_idx]->getSysID();
            }
            if (cpubase >= 0) {
                cpu_set_t mask;
                CPU_ZERO( &mask );
                CPU_SET( cpubase + sproc_idx, &mask );
                SYSCALL(pthread_setaffinity_np(tid, sizeof(cpu_set_t), &mask));
#if !TESTING_POLLER_FIBRES
                if (sproc_idx % cluster_size == 0) {
          tid = cluster[cluster_idx]->getPoller().getSysID();
          CPU_ZERO( &mask );
          for (int i = 0; i < cluster_size; i += 1) {
              if (sproc_idx + i < thread_count) CPU_SET( cpubase + sproc_idx + i, &mask );
          }
          SYSCALL(pthread_setaffinity_np(tid, sizeof(cpu_set_t), &mask));
        }
#endif
            }
        }

        // handle SIGINT to close the main socket
        std::call_once(signalRegisterFlag, signal, SIGINT, FibreServer::intHandler);

        // Start the timer to set the server date
        (new Fibre(*cluster[0], defaultStackSize))->run(FibreServer::timer, (void *)this);
    }

    ~FibreServer() {
        SYSCALL(lfClose(serverConnection));
    }

	// wrapper to pass to pool
	static void* handle_connection(void* arg){
		FibreHTTPSession::handle_connection(arg);
	}
    static void acceptor(void* arg) {
        FibreServer* fserver = (FibreServer*) arg;
        // StackPool pool(defaultStackSize);
        FibrePool pool;
        struct sockaddr_in serv_addr;
        while(true){
            socklen_t addrlen = sizeof(serv_addr);
            int fd = lfAccept(fserver->serverConnection, (sockaddr*)&serv_addr, &addrlen);
            if (fd >= 0)
                pool.start(FibreServer::handle_connection, (void*) new ServerAndFd(fserver, fd));
            else {
                assert(errno == ECONNRESET);
                std::cout << "ECONNRESET" << std::endl;
            }
        }
        pool.stop();
    }

    void start() {
        serverStarted.store(true);

        struct sockaddr_in serv_addr; //structure containing an internet address
        bzero((char*) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        serverConnection = SYSCALLIO(lfSocket(AF_INET, SOCK_STREAM, 0));
        int on = 1;
        SYSCALL(setsockopt(serverConnection, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(int)));
        // bind to server address
        if (SYSCALL(lfBind(serverConnection, (sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
            LOG_ERROR("Error on binding");
            exit(1);
        };
        // query and report addressing info
        socklen_t addrlen = sizeof(serv_addr);
        SYSCALL(getsockname(serverConnection, (sockaddr*)&serv_addr, &addrlen));
        FibreServer::servers.push_back(serverConnection);
        SYSCALL(lfListen(serverConnection, 65535));

        static const bool background = true;
        static const int ac_count = 2;
        Fibre* acfs[cluster_count][ac_count];
        for (int i = 0; i < cluster_count; i++) {
            for (int j = 0; j < ac_count; j++) {
                acfs[i][j] = new Fibre(*cluster[i], defaultStackSize, background);
                acfs[i][j]->run(acceptor, (void*)this);
            }
        }
        for (int i = 0; i < cluster_count; i++) {
            for (int j = 0; j < ac_count; j++) {
                delete acfs[i][j];
            }
        }
        SYSCALL(lfClose(serverConnection));
    };
};

};
#endif //NEWWEBSERVER_LIBFIBRESERVER_H
