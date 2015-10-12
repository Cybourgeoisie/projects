#ifndef P2PCLIENT_H
#define P2PCLIENT_H

#include "../common/P2PCommon.hpp"

using namespace std;

class P2PClient
{
	private:
		void initialize();

		// UI Methods
		void runProgram();
		bool runUI();
		char showMenu();
		void viewFiles();
		void selectFiles();
		vector<FileItem> collectFiles(vector<string>);
		void sendFiles(vector<FileItem>);

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