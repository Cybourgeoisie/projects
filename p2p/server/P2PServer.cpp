/**
 * Peer-to-peer server class
 */

#include "P2PServer.hpp"

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

// Client and file information
struct FileItem {
	unsigned int client_socket_id;
	string name;
	string hash;
	unsigned int size;
};


/**
 * Public Methods
 */

P2PServer::P2PServer()
{
	// Define the port number to listen through
	PORT_NUMBER = 27890;

	// Define server limits
	MAX_CLIENTS = 4;
	BUFFER_SIZE = 1025; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;

	// Keep track of the client sockets
	client_sockets = new int[MAX_CLIENTS];

	// Handle the buffer
	buffer = new char[BUFFER_SIZE];

	// Keep a list of the files
	vector<FileItem> file_list;

	// Default bind offset
	number_bind_tries = 1;
}

void P2PServer::setBindMaxOffset(unsigned int max_offset)
{
	number_bind_tries = max_offset;
}

//void P2PServer::setBindPort(string address) {}
//void P2PServer::setMaxClients(string address) {}

void P2PServer::start()
{
	// Open the socket and listen for connections
	this->initialize();
	this->openSocket();
	this->listenForClients();
}


/**
 * Private Methods
 */
void P2PServer::initialize()
{
	// Ensure that all client sockets are invalid to boot
	for (int i = 0; i < MAX_CLIENTS; i++) 
	{
		client_sockets[i] = 0;
	}
}

int P2PServer::bindToSocket(int socket, int offset)
{
	struct sockaddr_in server_address;

	// Clear out the server_address memory space
	memset((char *) &server_address, 0, sizeof(server_address));

	// Configure the socket information
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT_NUMBER + offset);

	return bind(socket, (struct sockaddr *) &server_address, sizeof(server_address));
}

void P2PServer::openSocket()
{
	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	primary_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (primary_socket < 0)
	{
		perror("Error: could not open socket");
		exit(1);
	}

	// Avoid the annoying "Address already in use" messages that the program causes
	int opt = 1;
	if (setsockopt(primary_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("Error: could not use setsockopt to set 'SO_REUSEADDR'");
        exit(1);
    }

	// Bind the socket
	port_offset = 0;
	int socket_status = -1;
	while (socket_status < 0)
	{
		// Bind to the given socket
		socket_status = this->bindToSocket(primary_socket, port_offset);

		// Stop eventually, if we can't bind to any ports
		if (socket_status < 0 && ++port_offset >= number_bind_tries)
		{
			perror("Fatal Error: Could not bind to any ports");
			exit(1);
		}
	}

	// Start listening on this port - second arg: max pending connections
	if (listen(primary_socket, MAX_CLIENTS) < 0)
    {
        perror("Error: could not listen on port");
        exit(1);
    }

	cout << "Listening on port " << (PORT_NUMBER + port_offset) << endl;
}

void P2PServer::resetSocketDescriptors()
{
	// Reset the current socket descriptors
	FD_ZERO(&socket_descriptors);
	FD_SET(primary_socket, &socket_descriptors);

	// Keep track of the maximum socket descriptor for select()
	max_client = primary_socket;
	 
	//add child sockets to set
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		// Validate the socket
		if (client_sockets[i] <= 0) continue;

		// Add socket to set
		FD_SET(client_sockets[i], &socket_descriptors);

		// Update the maximum socket descriptor
		max_client = max(max_client, client_sockets[i]);
	}
}

void P2PServer::handleNewConnection()
{
	// Prepare the client address
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);

	// Accept a new socket
	int new_socket = accept(primary_socket, (struct sockaddr *)&client_address, &client_address_length);

	// Validate the new socket
	if (new_socket < 0)
	{
		perror("Error: failure to accept new socket");
		exit(1);
	}

	// Report new connection
	cout << "New Connection Request: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;

	// Add socket to open slot
	for (int i = 0; i <= MAX_CLIENTS; i++) 
	{
		// If we reach the maximum number of clients, we've gone too far
		// Can't accept a new connection!
		if (i == MAX_CLIENTS)
		{
			// Report connection denied
			cout << "Reached maximum number of clients, denied connection request" << endl;

			// Send refusal message to socket
			string message = "Server is too busy, please try again later\r\n";
			write(new_socket, message.c_str(), message.length());

			close(new_socket);
			break;
		}

		// Skip all valid client sockets
		if (client_sockets[i] != 0) continue;

		// Add new socket
		client_sockets[i] = new_socket;
		break;
	}
}

void P2PServer::handleExistingConnections()
{
	// Prepare the client address
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);

	// Iterate over all clients
	for (int i = 0; i < MAX_CLIENTS; i++) 
	{
		if (!FD_ISSET(client_sockets[i], &socket_descriptors)) continue;

		// Clear out the buffer
		memset(&buffer[0], 0, sizeof(buffer));

		// Read the incoming message into the buffer
		int message_size = read(client_sockets[i], buffer, INCOMING_MESSAGE_SIZE);

		// Handle a closed connection
		if (message_size == 0)
		{
			// Report the disconnection
			getpeername(client_sockets[i], (struct sockaddr*)&client_address, &client_address_length);
			cout << "Connection closed: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;

			// Close and free the socket
			close(client_sockets[i]);
			client_sockets[i] = 0;
		}
		else
		{
			this->handleRequest(client_sockets[i], buffer);
		}
	}
}

void P2PServer::handleRequest(int client_socket, char* buffer)
{
	string request = string(buffer);
	request.erase(remove_if(request.begin(), request.end(), ::isspace), request.end());

	if (request.compare("register") == 0)
	{
		cerr << "Registering files" << endl;
	}
	else if (request.compare("list") == 0)
	{
		cerr << "Listing files" << endl;
	}
	else if (request.compare("get") == 0)
	{
		cerr << "Getting file" << endl;
	}
	else
	{
		cerr << "Request unknown: " << request << endl;
	}

	sleep(1);

	// Push a new file to the list
	/*FileItem file;
	file.name = string(buffer);
	file_list.push_back(file);

	cerr << "File list: " << file_list[0].name << endl;*/

	// Set the string terminating NULL byte on the end of the data read
	buffer[strlen(buffer)] = '\0';
	write(client_socket, buffer, strlen(buffer));
}

void P2PServer::listenForClients()
{
	while (true) 
	{
		// Ready the socket descriptors
		this->resetSocketDescriptors();
  
		// Wait for activity
		int activity = select(max_client + 1, &socket_descriptors, NULL, NULL, NULL);

		// Validate the activity
		if ((activity < 0) && (errno!=EINTR))
		{
			perror("Error: select failed");
			exit(1);
		}

		// Anything on the primary socket is a new connection
		if (FD_ISSET(primary_socket, &socket_descriptors))
		{
			this->handleNewConnection();
		}
		  
		// Perform any open activities on all other clients
		this->handleExistingConnections();
	}
}
