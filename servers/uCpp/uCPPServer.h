//
// Created by saman on 10/09/17.
//

#ifndef NEWWEBSERVER_UCPPSERVER_H
#define NEWWEBSERVER_UCPPSERVER_H
#include "utserver.h"
#include <uSocket.h>

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

namespace ucppserver {
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
	Acceptor( uSocketServer &socks, Server &server, UCPPServer *ucppserver ) : sockserver( socks ), server( server ), ucppserver(ucppserver), acceptor(socks) {
	} // Acceptor::Acceptor
}; // Acceptor
_Task Server {
	UCPPServer* ucppserver;
	uSocketServer &sockserver;
	Acceptor *terminate;
	int acceptorCnt;
	bool timeout;
  public:
	Server( uSocketServer &socks, UCPPServer *ucppserver  ) : sockserver( socks ), ucppserver(ucppserver), acceptorCnt( 1 ), timeout( false ) {
	} // Server::Server
	void connection() {
	} // Server::connection
	void complete( Acceptor *terminate, bool timeout ) {
		Server::terminate = terminate;
		Server::timeout = timeout;
	} // Server::complete
  private:
	void main() {
		new Acceptor( sockserver, *this, ucppserver );				// create initial acceptor
		for ( ;; ) {
			_Accept( connection ) {
				new Acceptor( sockserver, *this, ucppserver );		// create new acceptor after a connection
				acceptorCnt += 1;
			} or _Accept( complete ) {					// acceptor has completed with client
				delete terminate;						// delete must appear here or deadlock
				acceptorCnt -= 1;
		  if ( acceptorCnt == 0 ) break;				// if no outstanding connections, stop
				if ( timeout ) {
					new Acceptor( sockserver, *this, ucppserver );	// create new acceptor after a timeout
					acceptorCnt += 1;
				} // if
			} // _Accept
		} // for
	} // Server::main
}; // Server

void Acceptor::main() {
	try {
		server.connection();							// tell server about client connection
		handle_connection(ucppserver, this);
		server.complete( this, false );					// terminate
	} catch( uSocketAccept::OpenTimeout ) {
		server.complete( this, true );					// terminate
	} // try
} // Acceptor::main

// Ignore the timer for uC++
/*_PeriodicTask PeriodicHeaderDateTask{
public:
    PeriodicHeaderDateTask( uDuration period, HTTPServer* httpserver) :
        uPeriodicBaseTask(period), httpserver(httpserver){
        };
protected:
    void main(){
        HTTPServer::setDate(httpserver);
    }
private:
    HTTPServer* httpserver;
}*/
// uCPPHTTP Session
class UCPPHTTPSession : public utserver::HTTPSession {
 private:
    Acceptor *connection;
 public:
    UCPPHTTPSession(utserver::HTTPServer &s, Acceptor *c) : HTTPSession(s), connection(c) {};

    ssize_t recv(void *buf, size_t len, int flags) {
	ssize_t rlen;
	try {
		rlen = connection->acceptor.recv((char*)buf, len, flags);
    } catch( uSocketAccept::ReadFailure & rderr ) {
		int terrno = rderr.errNo();
		if ( terrno != EPIPE && terrno != ECONNRESET ) {
			std::cerr << "ERROR: URL read failure " << terrno << " " << strerror( terrno ) << std::endl;
			exit( EXIT_FAILURE );
	} // if
        // if we are here it means the connection is closed
	  rlen = 0;				// rlen not set for exception
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

// Function to handle connections for each http session
void handle_connection(void *arg1, void *arg2) {
    utserver::HTTPServer *server = (utserver::HTTPServer *) arg1;
    Acceptor *ccon = (Acceptor*) arg2;
    UCPPHTTPSession session(*server, ccon);
    session.serve();
}

class UCPPServer : public utserver::HTTPServer {
 protected:
    // number of clusters
    size_t cluster_count;

 public:
    UCPPServer(const std::string name, int p, int tc) : HTTPServer(name, p, tc){
		// TODO: fix timer
        HTTPServer::setDate(this);
   }

    ~UCPPServer() {
    }

    void start() {
        serverStarted.store(true);
		uSocketServer sockserver(port);
		{
			Server s(sockserver, this);
		}
    };
};

}; // namespace
#endif //NEWWEBSERVER_UCPPSERVER_H
