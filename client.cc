#include <grpcpp/grpcpp.h>
#include "service.grpc.pb.h"
#include <fstream>

#define CHUNK_SIZE (1 << 20)

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::Status;

std::string readFile(std::string fileName);

class FileTransferClient {
public:
	FileTransferClient(std::shared_ptr<Channel>& channel) : m_Stub(FileManager::NewStub(channel)) {}

	bool uploadFile(std::string& filename) {
		Reply reply;
		ClientContext ctx;
		std::unique_ptr<ClientWriter<Chunk>> writer(m_Stub->upload(&ctx, &reply));

		Chunk chunk;
		chunk.set_filename(filename);

		char* buffer = new char[CHUNK_SIZE];
		std::ifstream fileStream(filename, std::ios::binary);

		// Check if the file exists
		if(!fileStream.good()) {
			std::cout << "File not found!" << std::endl;
			return false;
		}

		// Stream it by reading chunks
		while(!fileStream.eof()) {
			fileStream.read(buffer, CHUNK_SIZE);
			chunk.set_buffer(std::string(buffer, fileStream.gcount()));
			writer->Write(chunk);
		}

		// Upload complete
		writer->WritesDone();
		Status status = writer->Finish();
		if(!status.ok()) {
			std::cout << status.error_message() << std::endl;
			return false;
		}

		std::cout << "Upload finished (" << reply.length() << " bytes)!" << std::endl;

		delete[] buffer;
		return true;
	}

	bool downloadFile(std::string& filename) {
		Chunk chunk;
		ClientContext ctx;
		Request request;
		request.set_name(filename);

		std::unique_ptr<grpc::ClientReader<Chunk>> reader(m_Stub->download(&ctx, request));
		std::ofstream fileStream;

		// Receieve download
		int len = 0;
		while(reader->Read(&chunk)) {
			// If file not open, prepend "down_" to filename and open it
			if(!fileStream.is_open()) {
				fileStream.open("down_" + filename, std::ios::binary | std::ios::trunc);
			}
			fileStream << *chunk.mutable_buffer();
			// Increment received bytes
			len += chunk.buffer().size();
		}

		std::cout << "Download finished (" << len << " bytes)!" << std::endl;
		reader->Finish();
		return true;
	}

private:
	std::unique_ptr<FileManager::Stub> m_Stub;
};


int main(int argc, char** argv) {
	std::string address("localhost:5000");
	std::string filename("newbg.jpg");

	switch(argc) {
		case 3: address = argv[2];
		case 2: filename = argv[1];
	}

	// SSL Certificate
	std::string cert = readFile("creds/server.crt");
    grpc::SslCredentialsOptions sslOptions;
    sslOptions.pem_root_certs = cert;

	// For insecure channel you can just use 'grpc::InsecureChannelCredentials()' and ignore the 3 lines above this
	std::shared_ptr<Channel> channel = grpc::CreateChannel(address, grpc::SslCredentials(sslOptions));
	FileTransferClient client(channel);
	
	// Upload test
	if(!client.uploadFile(filename)) {
		return 1;
	}

	// Download test
	if(!client.downloadFile(filename)) {
		return 1;
	}

	return 0;
}

std::string readFile(std::string filename) {
	std::ifstream file(filename);
	if(!file.is_open()) {
		std::cout << "File not found" << std::endl;
		return std::string();
	}
	return std::string(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
}
