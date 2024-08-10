#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

class Client {
private:
  int client_socket;

public:
  Client(std::string ip, int port);
};

#endif
