# gRPC SSL File Transfer

A C++ implementation of a secure client-server file transfer.

## Building

There is a make file which you can use to build the client and server.

Simply run `make` to compile.

After that you can have to create the certificates for ssl.

You can edit `creds/gen.sh` to configure the key gen or just run it to try it out. 

Requirements:
- gRPC library
- openssl to build the cert

For instructions on how to build grpc visit [this link](https://grpc.io/docs/languages/cpp/quickstart/).
