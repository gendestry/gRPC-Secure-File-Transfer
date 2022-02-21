LDFLAGS = -L$(HOME)/.local/lib `pkg-config --libs protobuf grpc++`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl

CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11

GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

# idk if necesarry
export PKG_CONFIG_PATH=$(HOME)/.local/lib/pkgconfig/

all: client server

client: service.pb.o service.grpc.pb.o client.o
	$(CXX) $^ $(LDFLAGS) -o $@

server: service.pb.o service.grpc.pb.o server.o
	$(CXX) $^ $(LDFLAGS) -o $@

rserver: server
	./server

rclient: client
	./client

%.grpc.pb.cc: %.proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	protoc --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h client server
