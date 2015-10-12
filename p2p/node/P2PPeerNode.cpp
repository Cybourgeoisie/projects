/**
 * Peer-to-peer node class
 */

#include "P2PPeerNode.hpp"

// Network Includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/**
 * Public Methods
 */

P2PPeerNode::P2PPeerNode()
{
	// Define the port number to listen through
	PORT_NUMBER = 27890;

	// Define server limits
	MAX_CONNECTIONS = 4;
	BUFFER_SIZE = 1025; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;

	// Keep track of the client sockets
	sockets = new int[MAX_CONNECTIONS];

	// Handle the buffer
	buffer = new char[BUFFER_SIZE];

	// Default bind offset
	number_bind_tries = 1;
}

void P2PPeerNode::setBindMaxOffset(unsigned int max_offset)
{
	number_bind_tries = max_offset;
}

//void P2PPeerNode::setBindPort(string address) {}
//void P2PPeerNode::setMaxClients(string address) {}

void P2PPeerNode::start()
{
	// Open the socket and listen for connections
	this->initialize();
	this->openPrimarySocket();
}


/**
 * Private Methods
 */
void P2PPeerNode::initialize()
{
	// Ensure that all sockets are invalid to boot
	for (int i = 0; i < MAX_CONNECTIONS; i++)
	{
		sockets[i] = 0;
	}
}

/**
 * Primary Socket
 * - Open and bind a socket to serve
 */

void P2PPeerNode::openPrimarySocket()
{
	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	primary_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (primary_socket < 0)
	{
		perror("Error: could not open primary socket");
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
		socket_status = this->bindPrimarySocket(primary_socket, port_offset);

		// Stop eventually, if we can't bind to any ports
		if (socket_status < 0 && ++port_offset >= number_bind_tries)
		{
			perror("Fatal Error: Could not bind primary socket to any ports");
			exit(1);
		}
	}

	// Start listening on this port - second arg: max pending connections
	if (listen(primary_socket, MAX_CONNECTIONS) < 0)
    {
        perror("Error: could not listen on port");
        exit(1);
    }

    P2PSocket a_socket;
	a_socket.socket_id = primary_socket;
	a_socket.type = "primary";
	socket_vector.push_back(a_socket);

	cout << "Listening on port " << (PORT_NUMBER + port_offset) << endl;
}

int P2PPeerNode::bindPrimarySocket(int socket, int offset)
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


/**
 * Make a connection
 */
int P2PPeerNode::makeConnection(string host, int port)
{
	struct sockaddr_in server_address;
	struct hostent * server;

	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	int new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket < 0)
	{
		perror("Error: could not open socket");
		exit(1);
	}

	// Find the server host
	server = gethostbyname(host.c_str());
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
	server_address.sin_port = htons(port);

	// Connect to the server
	if (connect(new_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) 
	{
		perror("Error: could not connect to host");
		exit(1);
	}

	// Add socket to open slot
	for (int i = 0; i <= MAX_CONNECTIONS; i++) 
	{
		// If we reach the maximum number of clients, we've gone too far
		// Can't make a new connection!
		if (i == MAX_CONNECTIONS)
		{
			// Report connection denied
			cout << "Reached maximum number of connections, can't add new one" << endl;
			close(new_socket);
			return 0;
		}

		// Skip all valid client sockets
		if (sockets[i] != 0) continue;

		// Add new socket
		sockets[i] = new_socket;

		P2PSocket a_socket;
		a_socket.socket_id = new_socket;
		a_socket.type = "server";
		a_socket.name = "";
		socket_vector.push_back(a_socket);

		break;
	}

	cout << "Connected to server on port " << port << endl;

	return new_socket;
}

/**
 * Handle Connection Activity
 */

void P2PPeerNode::resetSocketDescriptors()
{
	// Reset the current socket descriptors
	FD_ZERO(&socket_descriptors);
	FD_SET(primary_socket, &socket_descriptors);

	// Keep track of the maximum socket descriptor for select()
	max_connection = primary_socket;
	 
	// Add remaining sockets
	for (int i = 0; i < MAX_CONNECTIONS; i++)
	{
		// Validate the socket
		if (sockets[i] <= 0) continue;

		// Add socket to set
		FD_SET(sockets[i], &socket_descriptors);

		// Update the maximum socket descriptor
		max_connection = max(max_connection, sockets[i]);
	}
}

void P2PPeerNode::handleNewConnectionRequest()
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
	for (int i = 0; i <= MAX_CONNECTIONS; i++) 
	{
		// If we reach the maximum number of clients, we've gone too far
		// Can't accept a new connection!
		if (i == MAX_CONNECTIONS)
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
		if (sockets[i] != 0) continue;

		// Add new socket
		sockets[i] = new_socket;

		P2PSocket a_socket;
		a_socket.socket_id = new_socket;
		a_socket.type = "client";
		a_socket.name = "";
		socket_vector.push_back(a_socket);

		break;
	}
}

void P2PPeerNode::handleExistingConnections()
{
	// Prepare the client address
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);

	// Iterate over all clients
	for (int i = 0; i < MAX_CONNECTIONS; i++) 
	{
		if (!FD_ISSET(sockets[i], &socket_descriptors)) continue;

		// Clear out the buffer
		memset(&buffer[0], 0, sizeof(buffer));

		// Read the incoming message into the buffer
		int message_size = read(sockets[i], buffer, INCOMING_MESSAGE_SIZE);

		// Handle a closed connection
		if (message_size == 0)
		{
			// Report the disconnection
			getpeername(sockets[i], (struct sockaddr*)&client_address, &client_address_length);
			cout << "Connection closed: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;

			// Close and free the socket
			close(sockets[i]);

			// Remove the socket from the vector
			vector<P2PSocket>::iterator iter;
			for (iter = socket_vector.begin(); iter != socket_vector.end(); )
			{
				if (iter->socket_id == sockets[i])
					iter = socket_vector.erase(iter);
				else
					++iter;
			}

			// Remove from the int array
			sockets[i] = 0;
		}
		else
		{
			// Remove the socket from the vector
			vector<P2PSocket>::iterator iter;
			for (iter = socket_vector.begin(); iter != socket_vector.end(); ++iter)
			{
				if (iter->socket_id == sockets[i])
				{
					if (iter->type.compare("server") == 0)
					{
						this->enqueueMessage(sockets[i], buffer);
					}
					else if (iter->type.compare("client") == 0)
					{
						this->enqueueMessage(sockets[i], buffer);
					}
				}
			}
		}
	}
}

void P2PPeerNode::listenForActivity()
{
	while (true) 
	{
		// Ready the socket descriptors
		this->resetSocketDescriptors();
  
		// Wait for activity
		int activity = select(max_connection + 1, &socket_descriptors, NULL, NULL, NULL);

		// Validate the activity
		if ((activity < 0) && (errno!=EINTR))
		{
			perror("Error: select failed");
			exit(1);
		}

		// Anything on the primary socket is a new connection
		if (FD_ISSET(primary_socket, &socket_descriptors))
		{
			this->handleNewConnectionRequest();
		}
		  
		// Perform any open activities on all other clients
		this->handleExistingConnections();
	}
}


/**
 * Program Logic
 */

void P2PPeerNode::sendMessageToSocket(string request, int socket)
{
	// Write the message to the server socket
	if (write(socket, request.c_str(), request.length()) < 0)
	{
		perror("Error: could not send message to server");
		exit(1);
	}
}

void P2PPeerNode::enqueueMessage(int client_socket, char* buffer)
{
	P2PMessage message;
	message.socket_id = client_socket;
	message.message = string(buffer);

	// Push to the queue
	message_queue.push_back(message);
}

int P2PPeerNode::countSockets()
{
	return socket_vector.size();
}

int P2PPeerNode::countQueueMessages()
{
	return message_queue.size();
}

P2PMessage P2PPeerNode::popQueueMessage()
{
	// Validation
	if (message_queue.size() <= 0)
	{
		perror("Error: can't pop a message from an empty queue");
		exit(1);
	}

	// Return a message
	P2PMessage message = message_queue[0];
	message_queue.erase(message_queue.begin());
	return message;
}
