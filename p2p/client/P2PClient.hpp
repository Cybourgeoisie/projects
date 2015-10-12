#ifndef P2PCLIENT_H
#define P2PCLIENT_H

class P2PClient
{
	private:
		void initialize();

		// UI Methods
		void runProgram();
		bool runUI();
		char showMenu();
		void viewFiles();

		// UI Management
		bool b_awaiting_response;

		// Thread worker functions
		static void * startActivityListenerThread(void *);

		// Keep track of the peer node
		P2PPeerNode node;
		int server_socket;

	public:
		P2PClient();
		void start();
};

#endif