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
		int  bindToSocket(int, int);

		// Server limits and port
		int PORT_NUMBER;
		int MAX_CLIENTS;
		int BUFFER_SIZE;
		int INCOMING_MESSAGE_SIZE;

		// Buffer
		char * buffer;

		// Sockets
		int primary_socket;
		int * client_sockets;
		int max_client;

		// Settings
		int port_offset;
		unsigned int number_bind_tries;

		// Socket descriptors used for select()
		fd_set socket_descriptors;

	public:
		P2PServer();
		void start();
		void setBindMaxOffset(unsigned int);
};

#endif