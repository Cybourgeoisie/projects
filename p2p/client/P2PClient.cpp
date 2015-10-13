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

			// Parse the request
			vector<string> request_parsed = P2PCommon::parseRequest(message.message);

			// Trim whitespace from the command
			request_parsed[0] = P2PCommon::trimWhitespace(request_parsed[0]);

			// If the request starts with a keyword, turn to a different direction
			if (request_parsed[0].compare("fileAddress") == 0)
			{
				prepareFileTransferRequest(request_parsed[1], request_parsed[2]);
			}
			else if (request_parsed[0].compare("fileRequest") == 0)
			{
				initiateFileTransfer(message.socket_id, request_parsed[1]);
			}
			else if (request_parsed[0].compare("initiateFileTransfer") == 0)
			{
				startTransferFile(request_parsed);
			}
			else if (request_parsed[0].compare("fileTransfer") == 0)
			{
				handleIncomingFileTransfer(request_parsed, message.message.c_str());
			}
			else
			{
				// Show response
				cout << message.message << endl;
			}

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

void P2PClient::prepareFileTransferRequest(string file_id, string address_pair)
{
	// Trim any whitespace
	file_id = P2PCommon::trimWhitespace(file_id);
	address_pair = P2PCommon::trimWhitespace(address_pair);

	if (file_id == "NULL" || address_pair == "NULL")
	{
		return;
	}

	// Parse the address
	vector<string> address = P2PCommon::parseAddress(address_pair);

	// Make the connection
	node.makeConnection(file_id, address[0], stoi(address[1]));

	// Send the file request
	node.requestFileTransfer(file_id, stoi(file_id));
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
			getFile();
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

	// Cleanse the input
	int i_option = stoi(option);

	// Submit the request
	b_awaiting_response = true;
	node.sendMessageToSocket("getFile\r\n" + to_string(i_option), server_socket);
}

void P2PClient::initiateFileTransfer(int socket_id, string file_id)
{
	// Get the desired file from the central server
	b_awaiting_file_transfer = true;
	node.sendMessageToSocket("getFileForTransfer\r\nfile:" + file_id 
		+ "\r\nsocket:" + to_string(socket_id), server_socket);
}

void P2PClient::startTransferFile(vector<string> request)
{
	// Get the File Item info from the file ID
	vector<string> file_id_info = P2PCommon::splitString(request[1], ':');
	vector<string> socket_id_info = P2PCommon::splitString(request[2], ':');

	int socket_id = stoi(P2PCommon::trimWhitespace(socket_id_info[1]));
	string path = P2PCommon::trimWhitespace(request[3]);

	// Get the filename from the path
	vector<string> path_info = P2PCommon::splitString(path, '/');
	string filename = path_info.back();

	// If the filename is greater than 255 characters, trim it accordingly
	if (filename.length() > P2PCommon::MAX_FILENAME_LENGTH)
	{
		vector<string> filename_info = P2PCommon::splitString(filename, '.');
		string filename_ext = filename_info.back();

		// Return a filename that fits
		filename = filename.substr(0, P2PCommon::MAX_FILENAME_LENGTH - 1 - filename_ext.length()) + '.' + filename_ext;
	}

	// Load in the file bit by bit
	ifstream input_stream(path, ios::binary);
	if (input_stream)
	{
		// Get length of file
		input_stream.seekg(0, input_stream.end);
		unsigned int length = input_stream.tellg();

		// Determine number of file chunks
		unsigned int num_chunks;
		if (length % FILE_CHUNK_SIZE == 0)
			num_chunks = length/FILE_CHUNK_SIZE;
		else
			num_chunks = length/FILE_CHUNK_SIZE + 1;

		// Start back at beginning
		input_stream.seekg(0, input_stream.beg);

		// Allocate memory for sending data in chunks
		char * buffer = new char[FILE_CHUNK_SIZE + HEADER_SIZE];

		// Read data as blocks
		unsigned int i = 1;
		while (i <= num_chunks)
		{
			// Clear out the buffer
			memset(buffer, 0, FILE_CHUNK_SIZE + HEADER_SIZE);

			/*
				Header:
					flag (12) + 2 = 13 chars
					filename (255) + 1 = 256 chars
					total number of chunks (10) + 1, current chunk (10)  + 1 = 22 chars
					checksum (32) + 2 = 34 chars
					payload (1024 B)
			*/
			// \t\r\n + filename + flag + chunks  + checksum = header
			// 7      + 255      + 12   + 10 + 10 + 32       = 326
			sprintf(buffer, "%12s\r\n%255s\t%10d\t%10d\t%32s\r\n", 
				"fileTransfer", filename.c_str(), num_chunks, i, "hash");

			// Read in the data to send
			input_stream.read(&buffer[HEADER_SIZE], FILE_CHUNK_SIZE);
			int bytes_read = input_stream.gcount();

			// Send the data across the wire
			int bytes_written = write(socket_id, buffer, HEADER_SIZE + bytes_read);
			if (bytes_written < HEADER_SIZE + bytes_read)
			{
				// TO BE HANDLED - when write could not deliver the full payload
			}

			// If we write too fast, then the messages are sent in one packet
			usleep(5000);

			// Move to the next chunk
			input_stream.seekg((i++) * FILE_CHUNK_SIZE);
		}

		// Close and deallocate
		input_stream.close();
		delete[] buffer;
	}
}

void P2PClient::handleIncomingFileTransfer(vector<string> request, const char * raw_message)
{
	// Parse the info
	vector<string> header_info = P2PCommon::splitString(request[1], '\t');

	// Direct the data stream into the correct file
	string filename = P2PCommon::trimWhitespace(header_info[0]);
	string total_file_parts = P2PCommon::trimWhitespace(header_info[1]);
	string file_part = P2PCommon::trimWhitespace(header_info[2]);

	// Data folder
	string data_folder = "P2PRawData";

	// Ensure we have the data storage folder to work with
	struct stat s;
	int file_status = stat(data_folder.c_str(), &s);
	if (!(file_status == 0 && (s.st_mode & S_IFDIR)))
	{
		if (mkdir(data_folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
		{
			string message = "Error: could not create folder to save data. Create folder called " + data_folder;
			perror(message.c_str());
			return;
		}
	}

	// Get a truncated version of the filename
	string filename_addtl_ext = ".pt." + file_part + ".of." + total_file_parts + ".p2pft";
	string new_filename;
	string filename_trunc = filename;
	if (filename.length() >= P2PCommon::MAX_FILENAME_LENGTH - filename_addtl_ext.length())
	{
		filename_trunc = filename.substr(0, P2PCommon::MAX_FILENAME_LENGTH - filename_addtl_ext.length());
	}
	new_filename = filename_trunc + filename_addtl_ext;

	// Save the piece to this folder
	string data_filename = data_folder + "/" + new_filename;
	std::ofstream outfile(data_filename.c_str(), ios::binary);
	if (outfile)
	{
		// Write and close
		outfile.write(&raw_message[HEADER_SIZE], strlen(&raw_message[HEADER_SIZE]));
		outfile.flush();
		outfile.close();
	}
	else
	{
		perror("Error: could not open file to write");
	}

	// Once we've saved the data, analyze to see if we have a full file
	// Add all files in the directory
	DIR *directory;
	struct dirent *ep;

	// Open the directory
	directory = opendir(data_folder.c_str());
	if (directory != NULL)
	{
		// Get the expected number of parts
		unsigned int expected_num_parts = stoi(total_file_parts);
		unsigned int i = 0;

		// See how many parts we have available
		while ((ep = readdir(directory)) != NULL && expected_num_parts >= i)
		{
			// Evaluate that each part is there
			string this_filename = string(ep->d_name);
			if (this_filename.compare(0, filename_trunc.length(), filename_trunc) == 0)
			{
				i++;
			}
		}

		// If we have all the parts, combine them
		if (i == expected_num_parts)
		{
			string final_filename = P2PCommon::renameDuplicateFile(filename);
			std::ofstream outfile(final_filename.c_str(), ios::binary);
			if (outfile)
			{
				for (i = 1; i <= expected_num_parts; i++)
				{
					// Get the current file to read
					string this_filename = data_folder + '/' + filename_trunc 
						+ ".pt." + to_string(i) + ".of." + total_file_parts + ".p2pft";

					// Open the file to read in
					ifstream input_stream(this_filename, ios::binary);
					if (input_stream)
					{
						// Get length of file
						input_stream.seekg(0, input_stream.end);
						unsigned int length = input_stream.tellg();
						input_stream.seekg(0, input_stream.beg);

						// Allocate memory for transferring data
						char * buffer = new char[length];
						input_stream.read(buffer, length);

						// Read the contents of this file into the general file
						outfile.write(buffer, length);
						outfile.flush();
					}

					// Delete the unneeded chunk
					if (remove(this_filename.c_str()) != 0)
					{
						perror("Error: Could not remove outdated file chunk");
					}
				}

				// Close the output file
				outfile.close();
			}
			else
			{
				perror("Error: could not open file to write");
			}
		}

		// Close the directory
		closedir(directory);
	}
}

