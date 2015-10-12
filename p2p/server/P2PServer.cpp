/**
 * Peer-to-peer server class
 */

#include "../node/P2PPeerNode.hpp"
#include "../node/P2PPeerNode.cpp"
#include "P2PServer.hpp"

/**
 * Public Methods
 */

P2PServer::P2PServer() {}

void P2PServer::start()
{
	// Open the socket and listen for connections
	node = P2PPeerNode();
	node.setBindMaxOffset(1);
	node.start();

	// Once we're connected, we'll want to bind the server listener
	// Perform this as a separate thread
	pthread_t node_thread;
	if (pthread_create(&node_thread, NULL, &P2PServer::startActivityListenerThread, (void *)&node) != 0)
	{
		perror("Error: could not spawn peer listeners");
		exit(1);
	}

	// Now open up the menu
	runProgram();
}

void * P2PServer::startActivityListenerThread(void * arg)
{
	P2PPeerNode * node;
	node = (P2PPeerNode *) arg;
	node->listenForActivity();
	pthread_exit(NULL);
}

void P2PServer::runProgram()
{
	bool b_program_active = true;
	while (b_program_active)
	{
		// Handle updates
		if (socketsModified())
		{
			// Update socket-dependent logic
			updateFileList();
		}

		if (node.countQueueMessages() > 0)
		{
			// Get the oldest message
			P2PMessage message = node.popQueueMessage();

			// Handle message
			handleRequest(message.socket_id, message.message);
		}
		else
		{
			usleep(5000);
		}
	}
}

bool P2PServer::socketsModified()
{
	timeval node_sockets_lm = node.getSocketsLastModified();
	return (sockets_last_modified.tv_sec != node_sockets_lm.tv_sec || 
			sockets_last_modified.tv_usec != node_sockets_lm.tv_usec);
}

void P2PServer::updateFileList()
{
	// Get all of the current sockets
	vector<P2PSocket> sockets = node.getSockets();

	// Prep work
	bool b_socket_found;
	vector<FileItem>::iterator file_iter;
	vector<P2PSocket>::iterator sock_iter;

	// For each file, ensure that the file's owner is online
	for (file_iter = file_list.begin(); file_iter < file_list.end(); )
	{
		b_socket_found = false;

		for (sock_iter = sockets.begin(); sock_iter < sockets.end(); sock_iter++)
		{
			if ((*sock_iter).socket_id == (*file_iter).socket_id)
			{
				b_socket_found = true;
				break;
			}
		}

		if (b_socket_found)
			file_iter++;
		else
			file_list.erase(file_iter);
	}

	// Update the last modified time
	sockets_last_modified = node.getSocketsLastModified();
}

vector<string> P2PServer::parseRequest(string request)
{
	// Remove spaces from the request
	stringstream ss(request);
	vector<string> request_parsed;

	if (request.length() > 0)
	{
		string line;
		while (getline(ss, line, '\n'))
		{
			line.erase(line.find_last_not_of(" \n\r\t")+1);
			request_parsed.push_back(line);
		}
	}

	return request_parsed;
}

void P2PServer::handleRequest(int socket, string request)
{
	// Parse the request
	vector<string> request_parsed = parseRequest(request);	

	// Parse the request for a matching command
	if (request_parsed[0].compare("addFiles") == 0)
	{
		cerr << "Adding files" << endl;

		string message = addFiles(socket, request_parsed);
		write(socket, message.c_str(), message.length());
	}
	else if (request_parsed[0].compare("list") == 0)
	{
		cerr << "Listing files" << endl;

		string files = listFiles();
		write(socket, files.c_str(), files.length());
	}
	else if (request.compare("get") == 0)
	{
		cerr << "Getting file" << endl;
	}
	else
	{
		cerr << "Request unknown: " << request << endl;
	}
}

string P2PServer::addFiles(int socket, vector<string> files)
{
	int i = 0;
	vector<string>::iterator iter;
	for (iter = files.begin() + 1; iter < files.end(); iter++)
	{
		i++;

		std::stringstream line(*iter);
		std::string segment;
		std::vector<std::string> seglist;
		while(std::getline(line, segment, '\t'))
		{
			seglist.push_back(segment);
		}

		FileItem file_item;
		file_item.name = seglist[0];
		file_item.size = stoi(seglist[1]);
		file_item.socket_id = socket;

		file_list.push_back(file_item);
	}

	return to_string(i) + " files successfully added to file listing.";
}

string P2PServer::listFiles()
{
	if (file_list.size() == 0)
	{
		return "\r\nThere are currently no files stored on the server.\r\n";
	}

	int i = 0;
	string files_message = "\r\nFile Listing:\r\n";

	vector<FileItem>::iterator iter;
	for (iter = file_list.begin(); iter < file_list.end(); iter++)
	{
		files_message += "\t" + to_string(++i) + ") " + (*iter).name + " - (" + to_string((*iter).size) + " B)" "\r\n";
	}

	return files_message;
}

