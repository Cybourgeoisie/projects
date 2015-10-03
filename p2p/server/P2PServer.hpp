#ifndef P2PSERVER_H
#define P2PSERVER_H

class P2PServer
{
	private:
		void initialize();
		void openSocket();
		void resetSocketDescriptors();
		void handleNewConnection();
		void handleExistingConnections();
		void handleRequest(int, char*);
		void listenForClients();

	public:
		P2PServer();
		void start();
};

#endif