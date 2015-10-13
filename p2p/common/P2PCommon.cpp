/**
 * Peer-to-peer server class
 */

#include "P2PCommon.hpp"

P2PCommon::P2PCommon() {}

vector<string> P2PCommon::parseRequest(string request)
{
	return P2PCommon::splitString(request, '\n');
}

vector<string> P2PCommon::parseAddress(string request)
{
	return P2PCommon::splitString(request, ':');
}

vector<string> P2PCommon::splitString(string request, char delimeter)
{
	// Remove spaces from the request
	stringstream ss(request);
	vector<string> request_parsed;

	if (request.length() > 0)
	{
		string line;
		while (getline(ss, line, delimeter))
		{
			line.erase(line.find_last_not_of(" \n\r\t")+1);
			request_parsed.push_back(line);
		}
	}

	return request_parsed;
}
