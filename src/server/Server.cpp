#ifndef SERVER_HPP
#define SERVER_HPP

#define PROTO_IMPL
#include "proto.hpp"
#include <stddef.h>
constexpr int MAX_CLIENTS = 256;
constexpr int DEFAULT_PORT = 6969;

class Server {
  public:
    Server() : _port(DEFAULT_PORT) {};
    Server(int port);

    void start_up();
    void run();
    void handle_errors(const char *msg, int &arg);
    void broadcast_msg(int sender, char *msg, size_t len);
    void broadcast_connection(int new_client, const char *msg);
    void get_client_details(int fd, int i, const char *username_buff);
    void client_disconnect(int fd, int index);
    void send_dm();

  private:
    int _port;
    int _server_fd = -1;
};

#endif // !SERVER_HPP
