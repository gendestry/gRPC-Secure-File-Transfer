#include <grpcpp/grpcpp.h>
#include "service.grpc.pb.h"
#include <fstream>

#define CHUNK_SIZE (1 << 20)

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;

std::string readFile(std::string filename);

class FileManagerImpl final : public FileManager::Service {
	Status upload(ServerContext* context, ServerReader<Chunk>* reader, Reply* response) override {
		Chunk chunk;
		std::ofstream fileStream;

		// Start receiving
		int len = 0;
		while(reader->Read(&chunk)) {
			// If file hasnt been opened yet, open it and prepend "up_"
			if(!fileStream.is_open()) {
				std::string filename = "up_" + *chunk.mutable_filename();
				fileStream.open(filename, std::ios::binary | std::ios::trunc);
			}
			
			fileStream << *chunk.mutable_buffer();
			len += chunk.buffer().size();
		}

		fileStream.close();
		// Set response's length field to the number of bytes received
		response->set_length(len);

		std::cout << "Received " << len << " bytes!" << std::endl;
		return Status::OK;
	}

	Status download(ServerContext* context, const Request* request, ServerWriter<Chunk>* writer) override {
		std::ifstream fileStream(request->name(), std::ios::binary);
		
		// If file doesnt exist, return NOT_FOUND
		if(!fileStream.good()) {
			std::cout << "File " << request->name() << " not found!" << std::endl;
			return Status(grpc::StatusCode::NOT_FOUND, "File not found!");
		}

		Chunk chunk;
		chunk.set_filename(request->name());

		// Split file into chunks and send them
		int len = 0;
		char* buffer = new char[CHUNK_SIZE];
		while(!fileStream.eof()) {
			fileStream.read(buffer, CHUNK_SIZE);
			chunk.set_buffer(std::string(buffer, fileStream.gcount()));
			writer->Write(chunk);
			len += fileStream.gcount();
		}

		std::cout << "Transfered " << len << " bytes!" << std::endl;
		return Status::OK;
	}
};

int main(int argc, char** argv) {
	std::string address("localhost:5000");
	switch(argc) {
		case 2: address = argv[1];
	}

	// Create SSL connection
	std::string servercert = readFile("creds/server.crt");
    std::string serverkey = readFile("creds/server.key");

	grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp;
    pkcp.private_key = serverkey;
    pkcp.cert_chain = servercert;

	grpc::SslServerCredentialsOptions sslOptions;
    sslOptions.pem_root_certs = "";
    sslOptions.pem_key_cert_pairs.push_back(pkcp);

    std::shared_ptr<grpc::ServerCredentials> creds;
    creds = grpc::SslServerCredentials(sslOptions);

	// Creating the server
	ServerBuilder builder;
	FileManagerImpl service;

	// If you dont need ssl you can just use 'grpc::InsecureServerCredentials()' and igrnore the ssl stuff
	builder.AddListeningPort(address, creds);
	builder.RegisterService(&service);

	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on: " << address << std::endl;

	server->Wait();
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
