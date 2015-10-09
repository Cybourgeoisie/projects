/**
 * Peer-to-peer client class
 */

#include "P2PClient.hpp"

// Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <algorithm>

// Network Includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

// Define the port number to listen through
const int PORT_NUMBER = 27890;

// Sockets
int server_socket;

/**
 * Public Methods
 */

P2PClient::P2PClient() {}

void P2PClient::start()
{
	// Open the socket and listen for connections
	this->initialize();
	this->connectToHost();
	this->sendMessageToHost();
	this->closeSocketToHost();
}


/**
 * Private Methods
 */
void P2PClient::initialize() {}

void P2PClient::connectToHost()
{
	struct sockaddr_in server_address;
	struct hostent * server;

	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		perror("Error: could not open socket");
		exit(1);
	}

	// Find the server host
	server = gethostbyname("localhost");
	if (server == NULL)
	{
		perror("Error: could not find the host");
		exit(1);
	}

	// Clear out the server_address memory space
	memset((char *) &server_address, 0, sizeof(server_address));

	// Configure the socket information
	server_address.sin_family = AF_INET;
	memcpy(server->h_addr, &server_address.sin_addr.s_addr, server->h_length);
	server_address.sin_port = htons(PORT_NUMBER);

	// Connect to the server
	if (connect(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) 
	{
		perror("Error: could not connect to host");
		exit(1);
	}

	cout << "Connected to server on port " << PORT_NUMBER << endl;
}

void P2PClient::closeSocketToHost()
{
	close(server_socket);
}

void P2PClient::sendMessageToHost()
{
	cout << "Please enter the message: ";

	// Create and clear out the buffer
	char buffer[256];
	memset(&buffer, 0, sizeof(buffer));
	
	// Get the message from stdin
	fgets(buffer, sizeof(buffer) - 1, stdin);

	// Write the message to the server socket
	int n = write(server_socket, buffer, strlen(buffer));
	if (n < 0)
	{
		perror("ERROR writing to socket");
		memset(&buffer, 0, sizeof(buffer));
		exit(1);
	}

	// Read in the returned info
	n = read(server_socket, buffer, sizeof(buffer) - 1);
	if (n < 0) 
	{
		perror("ERROR reading from socket");
		exit(1);
	}

	// Print out returned info
	cout << buffer;
}
