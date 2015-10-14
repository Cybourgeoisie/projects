/**
 * Peer-to-peer file transfer class
 */

#include "P2PFileTransfer.hpp"

/**
 * Public Methods
 */

P2PFileTransfer::P2PFileTransfer() {}

void P2PFileTransfer::startTransferFile(char * raw_message)
{
	// Parse the request
	vector<string> request = P2PCommon::parseRequest(raw_message);

	// Get the File Item info from the file ID
	vector<string> file_id_info = P2PCommon::splitString(request[1], ':');
	vector<string> socket_id_info = P2PCommon::splitString(request[2], ':');

	int file_id = stoi(P2PCommon::trimWhitespace(file_id_info[1]));
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
					file id = 10 + 1 = 11 chars
					payload size = 5 + 1 = 6 chars
					total number of chunks (10) + 1, current chunk (10) + 1 = 22 chars
					checksum (8) + 2 = 34 chars
					payload (variable)
			// \t\r\n + file id + paysize + flag + chunks  + checksum = header
			// 8      + 10      + 5       + 12   + 10 + 10 + 8        = 63
			*/

			// Read in the data to send
			input_stream.read(&buffer[HEADER_SIZE], FILE_CHUNK_SIZE);
			int bytes_read = input_stream.gcount();

			// Prepend the header
			char header[HEADER_SIZE+1]; // +1 = null terminator
			sprintf(header, "%12s\r\n%10d\t%5d\t%10d\t%10d\t%8s\r\n", 
				"fileTransfer", file_id, bytes_read, num_chunks, i, "hash");

			// Copy the header to the buffer
			memcpy(&buffer[0], header, HEADER_SIZE);

			// Send the data across the wire
			int bytes_written = write(socket_id, buffer, HEADER_SIZE + bytes_read);
			if (bytes_written < HEADER_SIZE + bytes_read)
			{
				// TO BE HANDLED - when write could not deliver the full payload
			}

			// If we write too fast, then the messages are sent in one packet
			usleep(10000);

			// Move to the next chunk
			input_stream.seekg((i++) * FILE_CHUNK_SIZE);
		}

		// Close and deallocate
		input_stream.close();
		delete[] buffer;
	}
}

void P2PFileTransfer::handleIncomingFileTransfer(char * raw_message)
{
	// Parse the request
	vector<string> request = P2PCommon::parseRequest(raw_message);
	vector<string> header_info = P2PCommon::splitString(request[1], '\t');

	// Direct the data stream into the correct file
	string file_id = P2PCommon::trimWhitespace(header_info[0]);
	int payload_size = stoi(P2PCommon::trimWhitespace(header_info[1]));
	string total_file_parts = P2PCommon::trimWhitespace(header_info[2]);
	string file_part = P2PCommon::trimWhitespace(header_info[3]);

	string filename = "test.txt";

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
		outfile.write(&raw_message[HEADER_SIZE], payload_size);
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

