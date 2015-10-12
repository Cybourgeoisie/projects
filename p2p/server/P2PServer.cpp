/**
 * Peer-to-peer server class
 */

#include "../node/P2PPeerNode.hpp"
#include "../node/P2PPeerNode.cpp"
#include "P2PServerNew.hpp"

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

P2PServer::P2PServer() {}

void P2PServer::start()
{
	// Keep a list of the files
	vector<FileItem> file_list;

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

void P2PServer::handleRequest(int socket, string request)
{
	// Remove spaces from the request
	request.erase(remove_if(request.begin(), request.end(), ::isspace), request.end());

	// Parse the request for a matching command
	if (request.compare("register") == 0)
	{
		cerr << "Registering files" << endl;
	}
	else if (request.compare("list") == 0)
	{
		cerr << "Listing files" << endl;

		string longstring = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc faucibus ultricies elementum. Vestibulum commodo vulputate fermentum. Vestibulum dignissim lectus quis diam mollis tristique. Donec ultrices magna vel sem ullamcorper malesuada. Morbi justo lorem, maximus eu fringilla sit amet, mollis eleifend dui. Curabitur ligula arcu, posuere quis nulla quis, fermentum tempus erat. Curabitur pharetra est a mattis condimentum. Fusce tempor nulla eu sem vulputate rhoncus eu bibendum risus. Sed erat lorem, fringilla vel lorem dapibus, tristique auctor eros. Vestibulum in rutrum arcu. Vivamus vel arcu sed metus sodales viverra ac id velit. Sed semper iaculis risus ut dignissim. Donec in metus diam. Etiam ac ante urna. Quisque vel magna luctus, consequat odio ut, tempor mi. \r\n Vivamus pretium leo nec sollicitudin pharetra. Phasellus tristique vel elit ac sollicitudin. Sed vitae vestibulum arcu. Nunc consectetur ex sit amet justo imperdiet malesuada nec ut sem. Aliquam erat volutpat. In hac habitasse platea dictumst. Ut eget justo non sem cursus vehicula. Etiam accumsan metus non turpis tempor, vitae semper nulla suscipit. Curabitur laoreet pretium est, et rhoncus justo vulputate at. Duis varius mi at sem viverra vestibulum. Sed et varius massa. Maecenas nec euismod magna, malesuada elementum magna. Mauris ut sem eleifend, porttitor nisi vitae, consectetur urna. \r\n Ut laoreet lectus mauris, ac vehicula nulla aliquet a. Pellentesque dapibus enim at arcu euismod dictum. Nunc fermentum nisl non nunc fringilla euismod. Donec rutrum metus id diam cursus, non auctor diam iaculis. Fusce rutrum quam eget dui sollicitudin, vulputate congue diam mollis. Duis ut leo id nulla ullamcorper fermentum. Ut sit amet lacus eget est rhoncus maximus et vel metus. Praesent ut pretium mauris, sit amet lacinia nisi. Sed vel scelerisque libero. Donec massa leo, rhoncus sed auctor non, euismod suscipit mauris. Sed pretium luctus vehicula. Integer ante sapien, feugiat vel luctus in, finibus et eros. Morbi iaculis, mauris in elementum tempor, mauris mauris scelerisque ligula, sed vulputate ipsum lacus eget dui. Nulla vitae feugiat lacus, eu volutpat nisi. Nulla tempor nunc vestibulum lectus vulputate, ut interdum est consectetur. \r\n In at nibh efficitur, malesuada magna eget, scelerisque dolor. Nullam euismod elit dapibus sapien dictum, nec rhoncus erat luctus. Etiam vel feugiat massa. In leo orci, scelerisque vitae justo id, porttitor efficitur lorem. Nullam nec nunc congue, porttitor felis nec, egestas justo. Pellentesque lobortis, metus vitae tempor vehicula, ante odio efficitur sapien, nec tincidunt sapien quam at lorem. Donec vestibulum ligula eu convallis suscipit. \r\n Nunc dictum laoreet diam et facilisis. Pellentesque auctor tellus id risus ultrices semper. Donec venenatis quam sit amet lobortis tincidunt. Quisque aliquam volutpat nibh. Mauris pharetra eleifend quam vitae tincidunt. In leo ex, vehicula at dolor in, condimentum blandit lorem. Nulla lacinia elit id viverra viverra. Vivamus non arcu consectetur, varius arcu eu, dictum erat. Vivamus sapien purus, congue id tincidunt vitae, feugiat sed enim. Pellentesque ut tellus et odio luctus efficitur ut eget lorem. Phasellus vel massa eget sem pellentesque hendrerit non id odio. Curabitur cursus sagittis ex, ut euismod ex rutrum nec. Nulla pharetra id sapien eget egestas. Donec sed volutpat magna. \r\n Aenean rutrum metus urna, non tincidunt ex sollicitudin a. Etiam quis ultricies magna, nec tristique lacus. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Integer semper elit sed vulputate hendrerit. Donec iaculis metus iaculis augue efficitur, non tempus leo malesuada. Sed vulputate eu erat mollis molestie. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum lacinia posuere eros in tincidunt. \r\n Aliquam erat volutpat. Cras dignissim fermentum ante, ut accumsan enim pellentesque tempor. Sed luctus mi in nulla placerat, ut cursus velit luctus. Mauris iaculis sapien leo. Curabitur venenatis elit nulla, at posuere turpis molestie vitae. Suspendisse interdum odio ac sapien egestas, vel consectetur arcu consequat. Nunc cursus ultrices justo eu volutpat. Sed egestas volutpat.";
		write(socket, longstring.c_str(), longstring.length());
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
