/**
 * Peer-to-peer client class
 */

#include "../node/P2PPeerNode.hpp"
#include "../node/P2PPeerNode.cpp"
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
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Multithreading
#include <pthread.h>

using namespace std;

/**
 * Public Methods
 */

P2PClient::P2PClient() {}

void P2PClient::start()
{
	// Open the socket and listen for connections
	node = P2PPeerNode();
	node.setBindMaxOffset(100);
	node.start();
	server_socket = node.makeConnection("localhost", 27890);

	// Once we're connected, we'll want to bind the server listener
	// Perform this as a separate thread
	pthread_t node_thread;
	if (pthread_create(&node_thread, NULL, &P2PClient::startActivityListenerThread, (void *)&node) != 0)
	{
		perror("Error: could not spawn peer listeners");
		exit(1);
	}

	// Now open up the menu
	runProgram();
}

void * P2PClient::startActivityListenerThread(void * arg)
{
	P2PPeerNode * node;
	node = (P2PPeerNode *) arg;
	node->listenForActivity();
	pthread_exit(NULL);
}

void P2PClient::runProgram()
{
	bool b_program_active = true;
	while (b_program_active)
	{
		if (node.countQueueMessages() > 0)
		{
			// Get the oldest message
			P2PMessage message = node.popQueueMessage();

			// Show response
			cout << message.message << endl;

			// If we think we've got everything, wait to be certain
			if (node.countQueueMessages() == 0 && b_awaiting_response)
			{
				usleep(50000);
			}

			// Determine if we've gotten the full response
			b_awaiting_response = (node.countQueueMessages() > 0);
		}
		else if (b_awaiting_response)
		{
			usleep(5000);
		}
		else
		{
			b_program_active = runUI();			
		}
	}
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
			selectFiles();
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
	string option;
	getline(cin, option, '\n');

	// Cast to a char
	return option[0];
}

void P2PClient::viewFiles()
{
	b_awaiting_response = true;
	node.sendMessageToSocket("list", server_socket);
}

bool isLineBreak(char c) { return isspace(c) && !isblank(c); }

void P2PClient::selectFiles()
{
	cout << endl;
	cout << "Please enter the locations of the folders or files you'd like to add." << endl;
	cout << "List each item on its own line, separated by a new line." << endl;
	cout << "When you are finished, hit Enter / Return:" << endl << endl;

	vector<string> files;
	string location;

	do
	{
		getline(cin, location, '\n');
		location.erase(remove_if(location.begin(), location.end(), isLineBreak), location.end());

		if (location.length() > 0)
			files.push_back(location);
	}
	while (location.length() > 0);

	// Beam the files up
	addFiles(files);
}

void P2PClient::addFiles(vector<string> files)
{
	b_awaiting_response = true;
	string add_files_message = "addFiles\r\n";

	// Send the files one by one
	vector<string>::iterator iter;
	for (iter = files.begin(); iter < files.end(); iter++)
	{
		add_files_message += *iter + "\r\n";
	}

	node.sendMessageToSocket(add_files_message, server_socket);
}



