CC = g++

CFLAGS = -c
LDFLAGS= -lportaudio

all: sirp_client sirp_server

sirp_client: client.o client_test.o
	$(CC) client.o client_test.o -o sirp_client $(LDFLAGS)

sirp_server: server.o server_test.o
	$(CC) server.o server_test.o -o sirp_server $(LDFLAGS)

server.o: src/server.cpp
	$(CC) $(CFLAGS) src/server.cpp

server_test.o: server_test.cpp
	$(CC) $(CFLAGS) server_test.cpp

client.o: src/client.cpp
	$(CC) $(CFLAGS) src/client.cpp

client_test.o: client_test.cpp
	$(CC) $(CFLAGS) client_test.cpp

clean:
	rm -rf *o sirp
