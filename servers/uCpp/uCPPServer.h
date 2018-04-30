//
// Created by saman on 10/09/17.
//

#ifndef NEWWEBSERVER_UCPPSERVER_H
#define NEWWEBSERVER_UCPPSERVER_H
#include "utserver.h"
#include <uSocket.h>
#include <uRealTime.h>

#define AUTO_SPAWN
// uC++ specific
 unsigned int uDefaultPreemption() {                     //timeslicing not required
	return 0;
} // uDefaultPreemption
unsigned int uDefaultSpin() {                           // kernel schedule-spinning off
	return 0;
} // uDefaultPreemption
unsigned int uMainStackSize() {                         // reduce,    default 500K
	return 60 * 1000;
} // uMainStackSize

enum { THREAD_STACK_SIZE = 16 * 1008 };

namespace ucppserver {
template<typename ACCEPTOR>
void handle_connection(void *arg1, void *arg2);
class UCPPServer;
_Task Server;

_Task Acceptor {
	UCPPServer* ucppserver;
	uSocketServer &sockserver;
	Server &server;
	void main();
  public:
    uSocketAccept acceptor;
	Acceptor( uSocketServer &socks, uCluster& clus, Server &server, UCPPServer *ucppserver ) :
        uBaseTask( clus, THREAD_STACK_SIZE ),sockserver( socks ),
        server( server ), ucppserver(ucppserver), acceptor(socks) {
	} // Acceptor::Acceptor
}; // Acceptor

_Task Server {
	UCPPServer* ucppserver;
	uSocketServer &sockserver;
    uCluster& clus;
	Acceptor *terminate;
	int acceptorCnt;
	bool timeout;
  public:
	Server( uSocketServer &socks, uCluster& clus, UCPPServer *ucppserver  ) :
        uBaseTask( clus, THREAD_STACK_SIZE ), sockserver( socks ), clus(clus),
        ucppserver(ucppserver), acceptorCnt( 1 ), timeout( false ) {
	} // Server::Server
	void connection() {
	} // Server::connection
	void complete( Acceptor *terminate, bool timeout ) {
		Server::terminate = terminate;
		Server::timeout = timeout;
	} // Server::complete
  private:
	void main() {
		new Acceptor( sockserver, clus, *this, ucppserver );				// create initial acceptor
		for ( ;; ) {
			_Accept( connection ) {
				new Acceptor( sockserver, clus, *this, ucppserver );		// create new acceptor after a connection
				acceptorCnt += 1;
			} or _Accept( complete ) {					// acceptor has completed with client
				delete terminate;						// delete must appear here or deadlock
				acceptorCnt -= 1;
		  if ( acceptorCnt == 0 ) break;				// if no outstanding connections, stop
				if ( timeout ) {
					new Acceptor( sockserver, clus, *this, ucppserver );	// create new acceptor after a timeout
					acceptorCnt += 1;
				} // if
			} // _Accept
		} // for
	} // Server::main
}; // Server

void Acceptor::main() {
	try {
		server.connection();							// tell server about client connection
		handle_connection<Acceptor>(ucppserver, this);
		server.complete( this, false );					// terminate
	} catch( uSocketAccept::OpenTimeout ) {
		server.complete( this, true );					// terminate
	} // try
} // Acceptor::main


_Task AcceptorWorker {
    uSocketServer &socket;
    UCPPServer* ucppserver;

    void main() {
    for ( ;; ) {
        acceptor.accept();				// accept client connection
        handle_connection<AcceptorWorker>(ucppserver, this);
        acceptor.close();				// close client connection
    } // for
    } // AcceptorWorker::main
  public:
    uSocketAccept acceptor;				// create acceptor but do not accept connection
    AcceptorWorker( uSocketServer &socket, uCluster &clus, UCPPServer* server) :
    uBaseTask( clus, THREAD_STACK_SIZE ), socket( socket ), acceptor( socket, false ), ucppserver(server){}
}; // AcceptorWorker

// uCPPHTTP Session
template<typename ACCEPTOR>
class UCPPHTTPSession : public utserver::HTTPSession {
 private:
    ACCEPTOR *connection;
 public:
    UCPPHTTPSession(utserver::HTTPServer &s, ACCEPTOR *c) : HTTPSession(s), connection(c) {};

    ssize_t recv(void *buf, size_t len, int flags) {
    ssize_t rlen;
    try {
        rlen = connection->acceptor.recv((char*)buf, len, flags);
    } catch( uSocketAccept::ReadFailure & rderr ) {
        int terrno = rderr.errNo();
        if ( terrno != EPIPE && terrno != ECONNRESET ) {
            std::cerr << "ERROR: URL read failure " << terrno << " " << strerror( terrno ) << std::endl;
            // exit( EXIT_FAILURE ); // keep going to avoid server failure
            // during expeirments
    } // if
      // if we are here it means the connection is closed
    rlen = 0;     // rlen not set for exception
    } // try
        return rlen;
    };

    ssize_t send(const void *buf, size_t len, int flags) {
        return connection->acceptor.send((char*)buf, len, flags);
    };

    int getFD() {
        return 0;
    };

    void yield() {
    };
};

// Set HTTP date and time
_PeriodicTask PeriodicHeaderDateTask{
public:
    PeriodicHeaderDateTask( uDuration period, utserver::HTTPServer& httpserver) :
        uPeriodicBaseTask(period), httpserver(httpserver){
        }; // PeriodicHeaderDateTask()
protected:
    void main(){
        utserver::HTTPServer::setDate(&httpserver);
    } // main()
private:
    utserver::HTTPServer& httpserver;
}; // PeriodicTask

// Function to handle connections for each http session
template<typename ACCEPTOR>
void handle_connection(void *arg1, void *arg2) {
    utserver::HTTPServer *server = (utserver::HTTPServer *) arg1;
    ACCEPTOR *ccon = (ACCEPTOR*) arg2;
    UCPPHTTPSession<ACCEPTOR> session(*server, ccon);
    session.serve();
}

cpu_set_t mask; // large -> do not put on stack
class UCPPServer : public utserver::HTTPServer {
 protected:
     uSocketServer *sockserver;
     // number of threads per cluster
     static const size_t cluster_size = 1;
    // number of clusters
    size_t cluster_count;

    std::vector<uCluster*> cluster;
    std::vector<uProcessor*> sproc;

    static void intHandler(int signal) {
        exit(0);
    }; // intHandler

 public:
    UCPPServer(const std::string name, int p, int tc, int cpubase) : HTTPServer(name, p, tc){

        cluster_count = ((thread_count - 1) / cluster_size) + 1; // ceiling-div
        cluster.reserve(cluster_count); // default already exists
        sproc.reserve(thread_count); // default already exists

        cluster.push_back(&uThisCluster());
        sproc.push_back(&uThisProcessor());

        int cluster_idx = 0;
        for (int sproc_idx = 0; sproc_idx < thread_count; sproc_idx += 1) {
            if (sproc_idx != 0) { // mainSP
                if (sproc_idx % cluster_size == 0) {
                    cluster_idx += 1;
                    cluster.push_back(new uCluster("Worker Cluster"));
                } // if
                sproc.push_back(new uProcessor(*cluster[cluster_idx]));
            } // if sproc_idx != 0
            if (cpubase >= 0) {
                CPU_ZERO( &mask );
                CPU_SET( cpubase + sproc_idx, &mask );
                sproc[sproc_idx]->setAffinity(mask);
            } // if cpubase
        }

        // signal handler
        signal(SIGINT, UCPPServer::intHandler);
   } // UCPPServer()

    ~UCPPServer() {
    } // ~UCPPServer()

    void start() {
        serverStarted.store(true);

        // Start the timer to set the server date
        PeriodicHeaderDateTask phdt(1, *this);

        sockserver = new uSocketServer(port);
        {
#ifdef AUTO_SPAWN
            std::vector<Server*> servers(cluster_count);
            for(size_t cl = 0 ; cl < cluster_count; cl += 1){
    			servers.push_back( new Server(*sockserver, *cluster[cl], this));
            }
            for(auto server: servers){
                delete server;
            }
#else // AUTO_SPAWN
            static const size_t USER_THREADS = 60000;
            AcceptorWorker** workers = new AcceptorWorker*[USER_THREADS];
            for ( int ut = 0; ut < USER_THREADS; ut += 1 ) { // user threads
                workers[ut] = new AcceptorWorker( *sockserver, *cluster[ut % cluster_count], this);
                if ( ut % 1000 == 0 ) uThisTask().yield(); // allow workers to start
            } // for

            for ( int ut = 0; ut < USER_THREADS; ut += 1 ) { // destroy user threads
                delete workers[ut];
            } // for
#endif // AUTO_SPAW
        } //
    } // start()
}; // UCPPServer

}; // namespace
#endif //NEWWEBSERVER_UCPPSERVER_H
