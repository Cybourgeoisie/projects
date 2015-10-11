/**
 * Peer-to-peer client class
 */

#include "../server/P2PServer.hpp"
#include "../server/P2PServer.cpp"
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

// Multithreading
#include <pthread.h>

using namespace std;

/**
 * Public Methods
 */

P2PClient::P2PClient()
{
	PORT_NUMBER = 27890;
	BUFFER_SIZE = 1025; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;
}

//void P2PServer::setCentralServerAddress(string address) {}

void P2PClient::start()
{
	// Open the socket and listen for connections
	this->initialize();
	this->connectToHost();

	// Once we're connected, we'll want to bind the server listener
	// Perform this as a separate thread
	pthread_t server_thread;
	if (pthread_create(&server_thread, NULL, &P2PClient::startServerThread, NULL) != 0)
	{
		perror("Error: could not spawn peer listeners");
		exit(1);
	}

	// Take a breather
	sleep(1);

	// Now open up the menu
	bool b_program_active = true;
	while (b_program_active)
	{
		b_program_active = runUI();
	}

	// Close down
	this->closeSocketToHost();
}

void * P2PClient::startServerThread(void * arg)
{
	P2PServer server;
	server.setBindMaxOffset(100);
	server.start();
	pthread_exit(NULL);
}

bool P2PClient::runUI()
{
	char option = showMenu();

	switch (option)
	{
		case 'v':
			viewFiles();
			break;
		case 'a':
			break;
		case 'd':
			break;
		case 'p':
			break;
		case 'q':
			break;
		default:
			cout << endl << "That selection is unrecognized. Please try again." << endl;
			break;
	}

	return (option != 'q');
}

char P2PClient::showMenu()
{
	cout << endl;
	cout << "Welcome to the P2P Network. What would you like to do?" << endl;
	cout << "Please select an item below by entering the corresponding character." << endl << endl;
	cout << "\t (v) View Files on the Server" << endl;
	cout << "\t (a) Add Files to the Server" << endl;
	cout << "\t (d) Download a File" << endl;
	cout << "\t (p) View Download Progress" << endl;
	cout << "\t (q) Quit" << endl;

	// Take the client's order
	char option;
	cin >> option;

	return option;
}

void P2PClient::viewFiles()
{
	sendMessageToHost("list");
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

void P2PClient::sendMessageToHost(string request)
{
	// Write the message to the server socket
	if (write(server_socket, request.c_str(), request.length()) < 0)
	{
		perror("Error: could not send message to server");
		exit(1);
	}

	// Create and clear out the buffer
	char buffer[BUFFER_SIZE];
	memset(&buffer, 0, sizeof(buffer));

	// Read the incoming message into the buffer
	int message_size = read(server_socket, buffer, INCOMING_MESSAGE_SIZE);
	while (message_size > 0)
	{
		cout << buffer;
		
		// Reset the buffer
		memset(&buffer, 0, sizeof(buffer));

		// Read any other messages...
		message_size = read(server_socket, buffer, INCOMING_MESSAGE_SIZE);
	}
}
