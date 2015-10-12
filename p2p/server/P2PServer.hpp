#ifndef P2PSERVER_H
#define P2PSERVER_H

#include "../common/P2PCommon.hpp"

using namespace std;

class P2PServer
{
	private:
		void initialize();
		void runProgram();
		void handleRequest(int, string);
		vector<string> parseRequest(string);
		string addFiles(int, vector<string>);
		string listFiles();
		void updateFileList();
		bool socketsModified();

		// Server limits and port
		int PORT_NUMBER;

		// Thread worker functions
		static void * startActivityListenerThread(void *);

		// Keep track of the peer node
		P2PPeerNode node;

		// Keep a list of the files and active clients
		vector<FileItem> file_list;
		timeval sockets_last_modified;

	public:
		P2PServer();
		void start();
};

#endif