#include <iostream>
#include "P2PServer.cpp"
using namespace std;

int main(int argc, const char* argv[])
{
	// Program welcome message
	cout << "P2P Server - startup requested" << endl;

	// Start up the server
	P2PServer server;
	server.start();

	return 0;
}
