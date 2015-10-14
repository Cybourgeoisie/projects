#ifndef P2PCOMMON_H
#define P2PCOMMON_H

// Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

// Directory Management
#include <dirent.h>

// Network Includes
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Multithreading
#include <pthread.h>

using namespace std;

struct P2PSocket {
	unsigned int socket_id;
	string type;
	string name;
};

struct P2PMessage {
	unsigned int socket_id;
	string message;
};

struct P2PDataPacket {
	unsigned int socket_id;
	unsigned int file_id;
	unsigned int total_packets;
	unsigned int packet_number;
	char * data;
};

struct FileItem {
	unsigned int socket_id;
	unsigned int file_id;
	unsigned int size;
	string public_address;
	unsigned int public_port;
	string name;
	string path;
	string hash;
};

class P2PCommon
{
	public:
		P2PCommon();
		static vector<string> parseRequest(string);
		static vector<string> parseAddress(string);
		static vector<string> splitString(string, char);
		static string trimWhitespace(string);
		static string renameDuplicateFile(string);

		// Some variables used throughout the program
		static const unsigned int MAX_FILENAME_LENGTH = 255;
};

#endif