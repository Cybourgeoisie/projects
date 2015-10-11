#ifndef P2PCLIENT_H
#define P2PCLIENT_H

class P2PClient
{
	private:
		void initialize();

		// UI Methods
		bool runUI();
		char showMenu();
		void viewFiles();

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