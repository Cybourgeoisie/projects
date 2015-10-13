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
			// Trim trailing whitespace
			line.erase(line.find_last_not_of(" \n\r\t")+1);

			// Trim leading whitespace
			if (line.length() > 0)
			{
				int first_non_whitespace = line.find_first_not_of(" \n\r\t");
				if (first_non_whitespace > 0)
				{
					line = line.substr(first_non_whitespace, line.length() - first_non_whitespace);
				}
			}

			request_parsed.push_back(line);
		}
	}

	return request_parsed;
}
