#include <iostream>
#include "P2PClient.cpp"
using namespace std;

int main(int argc, const char* argv[])
{
	// Program welcome message
	cout << "P2P Client - startup requested" << endl;

	// Start up the client server
	P2PClient client;
	client.start();

	return 0;
}
