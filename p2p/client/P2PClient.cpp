/**
 * Peer-to-peer client class
 */

#include "P2PClient.hpp"

using namespace std;

/**
 * Public Methods
 */

P2PClient::P2PClient() { }

void P2PClient::start()
{
	// Open the socket and listen for connections
	node = P2PPeerNode();
	node.setBindMaxOffset(100);
	node.start();
	server_socket = node.makeConnection("central_server", "localhost", 27890);

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
	// Clear the screen to boot
	P2PCommon::clearScreen();

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
			P2PCommon::clearScreen();
			viewFiles();
			break;
		case 'a':
			selectFiles();
			break;
		case 'd':
			getFile();
			P2PCommon::clearScreen();
			break;
		case 'p':
			P2PCommon::clearScreen();
			showProgress();
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

	// Clear the screen now
	P2PCommon::clearScreen();

	// Filter and collect files
	vector<FileItem> file_list;
	file_list = collectFiles(files);
	if (file_list.size() == 0)
	{
		cout << "No files were added. Check the locations of your files and folders before submitting." << endl;
		return;
	}

	// Beam the files up
	sendFiles(file_list);
}

vector<FileItem> P2PClient::collectFiles(vector<string> files)
{
	// For each item, if it's a folder, get all files in that folder
	// If it's a file, add it directly
	// If the item isn't found, drop it
	vector<FileItem> clean_files;
	vector<string>::iterator iter;
	struct stat s;

	for (iter = files.begin(); iter < files.end(); iter++)
	{
		if (stat((*iter).c_str(), &s) == 0)
		{
			if (s.st_mode & S_IFDIR)
			{
				// Add all files in the directory
				DIR *directory;
				struct dirent *ep;

				// Open the directory
				directory = opendir((*iter).c_str());
				if (directory != NULL)
				{
					// Collect all files
					while ((ep = readdir(directory)) != NULL)
					{
						string file_path = (*iter) + "/" + string(ep->d_name);
						if (stat(file_path.c_str(), &s) == 0 && (s.st_mode & S_IFREG))
						{
							// Get the absolute path
							char *real_path = realpath(file_path.c_str(), NULL);

							FileItem file_item;
							file_item.name = ep->d_name;
							file_item.size = s.st_size;
							file_item.path = string(real_path);
							clean_files.push_back(file_item);

							// Notify user
							cout << "Adding file: " << real_path << endl;
							free(real_path);
						}
					}

					// Close the directory
					closedir(directory);
				}
			}
			else if (s.st_mode & S_IFREG)
			{
				// Get the absolute path
				char *real_path = realpath((*iter).c_str(), NULL);
				
				FileItem file_item;
				file_item.name = (*iter);
				file_item.size = s.st_size;
				file_item.path = string(real_path);
				clean_files.push_back(file_item);

				// Notify user
				cout << "Adding file: " << real_path << endl;
				free(real_path);
			}
		}
		else
		{
			cout << "Could not find file or folder: " << (*iter) << endl;
		}
	}

	return clean_files;
}

void P2PClient::sendFiles(vector<FileItem> files)
{
	// Prepare the primary address
	struct sockaddr_in primary_address;
	socklen_t primary_address_length = sizeof(primary_address);

	// Get the public-facing port and IP address
	primary_address = node.getPrimaryAddress();
	string address = inet_ntoa(primary_address.sin_addr);
	uint port = ntohs(primary_address.sin_port);

	// Prepare the request
	b_awaiting_response = true;
	string add_files_message = "addFiles\r\n" + address + ":" + to_string(node.getPublicPort()) + "\r\n";

	// Separate each file with newlines
	vector<FileItem>::iterator iter;
	for (iter = files.begin(); iter < files.end(); iter++)
	{
		add_files_message += (*iter).name + '\t' + to_string((*iter).size) + '\t' + (*iter).path + "\r\n";
	}

	node.sendMessageToSocket(add_files_message, server_socket);
}

void P2PClient::getFile()
{
	cout << endl << "Enter the numeric key for the file you'd like to download: ";

	// Get the file ID
	string option;
	getline(cin, option, '\n');
	option = P2PCommon::trimWhitespace(option);

	if (option.length() == 0)
	{
		return;
	}

	// Cleanse the input
	int i_option = stoi(option);

	// Submit the request
	node.sendMessageToSocket("getFile\r\n" + to_string(i_option), server_socket);
}

void P2PClient::showProgress()
{
	cout << "File Progress:" << endl;

	// Find all files that were requested, get current progress:
	string progress = node.getFileProgress();
	cout << progress << endl;
}
