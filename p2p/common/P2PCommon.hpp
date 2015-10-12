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
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

// Directory Management
#include <dirent.h>

// OpenSSL
#include <openssl/md5.h>

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

struct FileItem {
	unsigned int socket_id;
	string name;
	string path;
	string hash;
	unsigned int size;
};

#endif