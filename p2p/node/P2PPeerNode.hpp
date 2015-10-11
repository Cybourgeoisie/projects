#ifndef P2PPEERNODE_H
#define P2PPEERNODE_H

// Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <algorithm>

using namespace std;

struct P2PSocket {
	unsigned int socket_id;
	string type;
	string handler;
	string name;
};

struct P2PMessage {
	unsigned int socket_id;
	string message;
};

class P2PPeerNode
{
	private:
		void initialize();
		void resetSocketDescriptors();
		void handleNewConnectionRequest();
		void handleExistingConnections();
		void handleRequest(int, char*);

		void enqueueMessage(int, char*);

		// Own socket
		void openPrimarySocket();
		int  bindPrimarySocket(int, int);

		// Server limits and port
		int PORT_NUMBER;
		int MAX_CONNECTIONS;
		int BUFFER_SIZE;
		int INCOMING_MESSAGE_SIZE;

		// Buffer
		char * buffer;

		// Message queue
		vector<P2PMessage> message_queue;

		// Sockets
		int primary_socket;
		int * sockets;
		int max_connection;

		// Available sockets
		vector<P2PSocket> socket_vector;

		// Settings
		int port_offset;
		unsigned int number_bind_tries;

		// Socket descriptors used for select()
		fd_set socket_descriptors;

	public:
		P2PPeerNode();
		void start();
		void listenForActivity();
		void setBindMaxOffset(unsigned int);

		// Add new connections
		int makeConnection(string, int);
		int countSockets();

		// Send message to socket
		void sendMessageToSocket(string, int);

		// Interact with message queue
		int  countQueueMessages();
};

#endif