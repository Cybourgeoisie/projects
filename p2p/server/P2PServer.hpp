#ifndef P2PSERVER_H
#define P2PSERVER_H

using namespace std;

struct FileItem {
	unsigned int socket_id;
	string name;
	string hash;
	unsigned int size;
};

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

		// Server limits and port
		int PORT_NUMBER;

		// Thread worker functions
		static void * startActivityListenerThread(void *);

		// Keep track of the peer node
		P2PPeerNode node;

		// Keep a list of the files and active clients
		vector<FileItem> file_list;
		vector<int> active_sockets;

	public:
		P2PServer();
		void start();
};

#endif