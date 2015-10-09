#ifndef P2PCLIENT_H
#define P2PCLIENT_H

class P2PClient
{
	private:
		void initialize();
		void connectToHost();
		void sendMessageToHost();
		void closeSocketToHost();

	public:
		P2PClient();
		void start();
};

#endif