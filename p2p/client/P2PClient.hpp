#ifndef P2PCLIENT_H
#define P2PCLIENT_H

class P2PClient
{
	private:
		void initialize();
		void connectToHost();
		void sendMessageToHost(string);
		void closeSocketToHost();
		bool runUI();
		char showMenu();
		void viewFiles();

		// Thread worker functions
		static void * startServerThread(void *);

		// Buffer
		int BUFFER_SIZE;
		int INCOMING_MESSAGE_SIZE;

		// Define the port number to listen through
		int PORT_NUMBER;
		int server_socket;

	public:
		P2PClient();
		void start();
};

#endif