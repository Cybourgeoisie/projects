#ifndef P2PFILETRANSFER_H
#define P2PFILETRANSFER_H

using namespace std;

class P2PFileTransfer
{
	private:
		vector<FileItem> file_list;

	public:
		P2PFileTransfer();

		void startTransferFile(FileDataPacket);
		void handleIncomingFileTransfer(FileDataPacket);
		string computeChecksum(char *, int);

		// File transfer
		static const unsigned int FILE_CHUNK_SIZE = 449;
		static const unsigned int HEADER_SIZE = 63;
};

#endif