/**
 * Peer-to-peer node class
 */

#include "P2PPeerNode.hpp"

/**
 * Public Methods
 */

P2PPeerNode::P2PPeerNode(int max_connections_value)
{
	this->construct(max_connections_value);
}

P2PPeerNode::P2PPeerNode()
{
	this->construct(4);
}

void P2PPeerNode::construct(int max_connections_value)
{
	// Define the port number to listen through
	PORT_NUMBER = 27890;

	// Define server limits
	MAX_CONNECTIONS = max_connections_value;
	BUFFER_SIZE = 513; // Size given in bytes
	INCOMING_MESSAGE_SIZE = BUFFER_SIZE - 1;

	// Keep track of the client sockets
	sockets = new int[MAX_CONNECTIONS];

	// Handle the buffer
	buffer = new char[BUFFER_SIZE];

	// Default bind offset
	number_bind_tries = 1;

	// Keep track of the last update to the sockets
	gettimeofday(&sockets_last_modified, NULL);
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

	// Set the public-facing port
	public_port = PORT_NUMBER + port_offset;

	P2PSocket a_socket;
	a_socket.socket_id = primary_socket;
	a_socket.type = "primary";
	socket_vector.push_back(a_socket);

	// Keep track of the last update to the sockets
	gettimeofday(&sockets_last_modified, NULL);

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
int P2PPeerNode::makeConnection(string name, string host, int port)
{
	struct sockaddr_in server_address;
	struct hostent * server;

	// Create the socket - use SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	int new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket < 0)
	{
		perror("Error: could not open socket");
		return -1;
	}

	// Find the server host
	server = gethostbyname(host.c_str());
	if (server == NULL)
	{
		perror("Error: could not find the host");
		return -1;
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
		return -1;
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
		a_socket.name = name;
		socket_vector.push_back(a_socket);

		// Keep track of the last update to the sockets
		gettimeofday(&sockets_last_modified, NULL);

		break;
	}

	cout << "Connected to server on port " << port << endl;

	return new_socket;
}

void P2PPeerNode::closeSocket(int socket)
{
	// Close and free the socket
	close(socket);

	// Remove the socket from the vector
	vector<P2PSocket>::iterator iter;
	for (iter = socket_vector.begin(); iter != socket_vector.end(); )
	{
		if (iter->socket_id == socket)
			iter = socket_vector.erase(iter);
		else
			++iter;
	}

	// Keep track of the last update to the sockets
	gettimeofday(&sockets_last_modified, NULL);

	// Remove from the int array
	for (int i = 0; i < MAX_CONNECTIONS; i++)
	{
		if (socket == sockets[i])
		{
			sockets[i] = 0;
			break;
		}
	}
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

		// Keep track of the last update to the sockets
		gettimeofday(&sockets_last_modified, NULL);

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
		memset(&buffer[0], 0, BUFFER_SIZE);

		// Read the incoming message into the buffer
		int message_size = read(sockets[i], buffer, INCOMING_MESSAGE_SIZE);

		// Handle a closed connection
		if (message_size == 0)
		{
			// Report the disconnection
			getpeername(sockets[i], (struct sockaddr*)&client_address, &client_address_length);
			cout << "Connection closed: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;

			// Close and free the socket
			closeSocket(sockets[i]);
		}
		else
		{
			// Make a copy of the vector to avoid concurrency issues
			vector<P2PSocket> socket_vector_copy;
			socket_vector_copy = socket_vector;

			// Remove the socket from the vector
			vector<P2PSocket>::iterator iter;
			for (iter = socket_vector_copy.begin(); iter != socket_vector_copy.end(); ++iter)
			{
				if (iter->socket_id == sockets[i])
				{
					// Parse the request
					vector<string> request_parsed = P2PCommon::parseRequest(buffer);

					// Trim whitespace from the command
					request_parsed[0] = P2PCommon::trimWhitespace(request_parsed[0]);

					if (request_parsed[0].compare("fileTransfer") == 0)
					{
						// Get the File ID
						vector<string> file_id_info = P2PCommon::splitString(request_parsed[1], ':');
						vector<string> header_info = P2PCommon::splitString(request_parsed[1], '\t');
						int file_id = stoi(P2PCommon::trimWhitespace(header_info[0]));

						// Make a copy of the data
						char * buffer_copy = new char[BUFFER_SIZE];
						memcpy(buffer_copy, buffer, BUFFER_SIZE);

						// Pack the data neatly for travel
						FileDataPacket packet;
						packet.file_item = getFileItem(file_id);
						packet.packet = buffer_copy;

						// Perform this as a separate thread
						pthread_t thread;
						if (pthread_create(&thread, NULL, &P2PPeerNode::handleFileTransfer, (void *)&packet) != 0)
						{
							perror("Error: could not spawn thread");
							//exit(1);
						}
					}
					else if (request_parsed[0].compare("initiateFileTransfer") == 0)
					{
						// Get the File ID
						vector<string> file_id_info = P2PCommon::splitString(request_parsed[1], ':');
						int file_id = stoi(P2PCommon::trimWhitespace(file_id_info[1]));

						// Make a copy of the data
						char * buffer_copy = new char[BUFFER_SIZE];
						memcpy(buffer_copy, buffer, BUFFER_SIZE);

						// Pack the data neatly for travel
						FileDataPacket packet;
						packet.file_item = getFileItem(file_id);
						packet.packet = buffer_copy;

						// Perform this as a separate thread
						pthread_t thread;
						if (pthread_create(&thread, NULL, &P2PPeerNode::initiateFileTransfer, (void *)&packet) != 0)
						{
							perror("Error: could not spawn thread");
							//exit(1);
						}
					}
					else if (request_parsed[0].compare("fileRequest") == 0)
					{
						getFileForTransfer(iter->socket_id, request_parsed[1]);
					}
					else if (request_parsed[0].compare("fileAddress") == 0)
					{
						prepareFileTransferRequest(request_parsed);
					}
					else if (iter->type.compare("server") == 0)
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

timeval P2PPeerNode::getSocketsLastModified()
{
	return sockets_last_modified;
}

vector<P2PSocket> P2PPeerNode::getSockets()
{
	return socket_vector;
}

struct sockaddr_in P2PPeerNode::getClientAddressFromSocket(int socket_id)
{
	// Prepare the client address
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);

	// Get the peer name
	getpeername(socket_id, (struct sockaddr*)&client_address, &client_address_length);

	return client_address;
}

struct sockaddr_in P2PPeerNode::getPrimaryAddress()
{
	return getClientAddressFromSocket(primary_socket);
}

int P2PPeerNode::getPublicPort()
{
	return public_port;
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

string P2PPeerNode::getFileProgress()
{
	string progress;

	// Iterate through all files in the local file list
	vector<FileItem>::iterator iter;
	for (iter = local_file_list.begin(); iter != local_file_list.end(); ++iter)
	{
		// RBH need to get actual progress
		progress += "\t" + (*iter).name + "\tCompleted" + "\r\n";
	}

	if (progress.length() == 0)
	{
		progress = "You have no active or past file transfers.";
	}

	return progress;
}

bool P2PPeerNode::hasSocketByName(string name)
{
	vector<P2PSocket>::iterator iter;
	for (iter = socket_vector.begin(); iter != socket_vector.end(); ++iter)
	{
		if ((*iter).name == name)
		{
			return true;
		}
	}

	return false;
}

P2PSocket P2PPeerNode::getSocketByName(string name)
{
	P2PSocket socket;
	vector<P2PSocket>::iterator iter;
	for (iter = socket_vector.begin(); iter != socket_vector.end(); ++iter)
	{
		if ((*iter).name == name)
		{
			socket = (*iter);
		}
	}

	return socket;
}

void P2PPeerNode::sendMessageToSocketName(string socket_name, string message)
{
	// Validate
	if (!hasSocketByName(socket_name))
	{
		return;
	}

	// Get a socket by name
	P2PSocket socket = getSocketByName(socket_name);
	if (write(socket.socket_id, message.c_str(), message.length()) < 0)
	{
		perror("Error: could not send message to server");
		exit(1);
	}
}

void P2PPeerNode::prepareFileTransferRequest(vector<string> request)
{
	// Trim any whitespace
	string file_id = P2PCommon::trimWhitespace(request[1]);
	string name = P2PCommon::trimWhitespace(request[2]);
	int size = stoi(P2PCommon::trimWhitespace(request[3]));
	string address_pair = P2PCommon::trimWhitespace(request[4]);

	// If the data came back invalid (possible race condition), just bail
	if (file_id == "NULL" || address_pair == "NULL")
	{
		return;
	}

	// Keep a record of this file
	FileItem file_item;
	file_item.name = name;
	file_item.size = size;
	file_item.file_id = stoi(file_id);

	// Push to our local cache
	local_file_list.push_back(file_item);

	// Parse the address
	vector<string> address = P2PCommon::parseAddress(address_pair);

	// Make the connection
	makeConnection(file_id, address[0], stoi(address[1]));

	// Send the file request
	requestFileTransfer(file_id, stoi(file_id));
}

void P2PPeerNode::requestFileTransfer(string socket_name, int file_id)
{
	// Compose the file transfer request
	string file_transfer_request = "fileRequest\r\n" + to_string(file_id);
	sendMessageToSocketName(socket_name, file_transfer_request);
}

void P2PPeerNode::getFileForTransfer(int socket_id, string file_id)
{
	// Get the desired file from the central server
	sendMessageToSocketName("central_server", "getFileForTransfer\r\nfile:" + file_id 
		+ "\r\nsocket:" + to_string(socket_id));
}

void * P2PPeerNode::initiateFileTransfer(void * arg)
{
	// Revive the packet
	FileDataPacket * packet;
	packet = (FileDataPacket *) arg;

	P2PFileTransfer file_transfer;
	file_transfer.startTransferFile(*packet);

	/* // Not possible inside a thread - actually, could package the socket info with the FileDataPacket..
	// Close the socket
	vector<string> request = P2PCommon::parseRequest(buffer);
	vector<string> socket_id_info = P2PCommon::splitString(request[2], ':');
	int socket_id = stoi(P2PCommon::trimWhitespace(socket_id_info[1]));
	closeSocket(socket_id);
	*/

	// Finished - free the buffer and close the thread
	delete[] (*packet).packet;
	pthread_exit(NULL);
}

void * P2PPeerNode::handleFileTransfer(void * arg)
{
	// Revive the packet
	FileDataPacket * packet;
	packet = (FileDataPacket *) arg;

	P2PFileTransfer file_transfer;
	file_transfer.handleIncomingFileTransfer(*packet);

	delete[] (*packet).packet;
	pthread_exit(NULL);
}

FileItem P2PPeerNode::getFileItem(int file_id)
{
	FileItem file_item;
	vector<FileItem>::iterator iter;
	for (iter = local_file_list.begin(); iter != local_file_list.end(); ++iter)
	{
		if ((*iter).file_id == file_id)
		{
			file_item = (*iter);
			break;
		}
	}
	return file_item;
}
