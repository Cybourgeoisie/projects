#ifndef P2PPEERNODE_H
#define P2PPEERNODE_H

#include "../common/P2PCommon.cpp"
#include "../filetransfer/P2PFileTransfer.cpp"

using namespace std;

class P2PPeerNode
{
	private:
		void construct(int);
		void initialize();
		void resetSocketDescriptors();
		void handleNewConnectionRequest();
		void handleExistingConnections();
		void handleRequest(int, char*);
		void enqueueMessage(int, char*);

		// Get File for transfer
		void prepareFileTransferRequest(vector<string>);
		void getFileForTransfer(int, string);
		FileItem getFileItem(int);

		// Worker threads
		static void * initiateFileTransfer(void *);
		static void * handleFileTransfer(void *);

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

		// Message queue and file list
		vector<P2PMessage> message_queue;
		vector<FileItem> local_file_list;

		// Sockets
		int primary_socket;
		int * sockets;
		int max_connection;

		// Managing Sockets
		timeval sockets_last_modified;

		// Available sockets
		vector<P2PSocket> socket_vector;

		// Settings
		int port_offset;
		int public_port;
		unsigned int number_bind_tries;

		// Socket descriptors used for select()
		fd_set socket_descriptors;

	public:
		P2PPeerNode();
		P2PPeerNode(int);
		void start();
		void listenForActivity();
		void setBindMaxOffset(unsigned int);

		// Add and remove new connections
		int makeConnection(string, string, int);
		void closeSocket(int);

		int countSockets();
		vector<P2PSocket> getSockets();
		timeval getSocketsLastModified();
		struct sockaddr_in getClientAddressFromSocket(int);
		struct sockaddr_in getPrimaryAddress();
		int getPublicPort();

		bool hasSocketByName(string);
		P2PSocket getSocketByName(string);
		void sendMessageToSocketName(string, string);

		// Progress
		string getFileProgress();

		// Send message to socket
		void sendMessageToSocket(string, int);
		void requestFileTransfer(string, int);

		// Interact with message queue
		P2PMessage popQueueMessage();
		int countQueueMessages();

		// Interact with data queue
		int countQueueData();
};

#endif