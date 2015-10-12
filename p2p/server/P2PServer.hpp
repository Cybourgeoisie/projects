#ifndef P2PSERVER_H
#define P2PSERVER_H

using namespace std;

class P2PServer
{
	private:
		void initialize();
		void runProgram();
		void handleRequest(int, string);

		// Server limits and port
		int PORT_NUMBER;

		// Thread worker functions
		static void * startActivityListenerThread(void *);

		// Keep track of the peer node
		P2PPeerNode node;

	public:
		P2PServer();
		void start();
};

#endif