#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

class Server {
private:
  int server_socket;
  sockaddr_in server_address;

public:
  Server(int port);
};

#endif
