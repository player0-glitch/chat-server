#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

using std::cout, std::cerr, std::endl;

constexpr int PORT = 10000;
constexpr int MAX_CLIENTS = 10;
constexpr int MAX_BUFF = 1058;
constexpr int MAX_USR_LEN = 32;
// globals
struct sockaddr_in server_addr;
unsigned int sockaddr_len;
int server_fd = -1;
int client_fd = -1;
int new_socket = -1;
int highest_fd = -1;
int sock_activity = 0;
int bytes_read = -1;

// holds all out active socker activity
fd_set master_set, current_set;
struct Client {
  int fd;
  char name[MAX_USR_LEN];
};
// containers
int clients[MAX_CLIENTS];
Client client_structs[MAX_CLIENTS];
char buff[MAX_BUFF]; // filled with newline values
std::unordered_map<int, Client> clients_map;
// function signatures
void handle_errors(const char *msg, int &arg);
void broadcast_msg(int sender, char *msg, size_t len);
void broadcast_connection(int new_client, char *msg, int name_len);
void get_client_details(int fd, int i, const char *username_buff);
void client_disconnect(int fd, int i);
int main() {
  // create a socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    handle_errors("Failed to create server socket", errno);
  else
    cout << "Successfully made server socket" << endl;

  // master server addrs
  server_addr = {AF_INET, htons(PORT), INADDR_ANY};
  sockaddr_len = sizeof(server_addr);
  // allows for socket to be reusable
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) != 0)
    handle_errors("Failed to make server reusable", server_fd);
  else
    cout << "Successfully made server reusable" << endl;

  // bind the socket
  if (bind(server_fd, (const struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0)
    handle_errors("Failed to bind socket to addr", server_fd);
  else
    cout << "Successfully binded socket to addr" << endl;

  // listen to connections
  if (listen(server_fd, 3) < 0)
    handle_errors("Failed to listen to connection", server_fd);
  else {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
    cout << "Server runnining on " << ip << ":" << ntohs(server_addr.sin_port)
         << endl;
  }
  // have a fd set for allowing multiple clients
  FD_ZERO(&master_set);
  FD_SET(server_fd, &master_set); // server is always on top
                                  //

  while (1) {
    // because select is destructive
    current_set = master_set;
    highest_fd = server_fd;
    /*char buff[MAX_BUFF];*/

    // add client (child) socket file descriptors into the set
    for (int i = 0; i < MAX_CLIENTS; i++) {
      client_fd = clients[i];
      // looking for a valid client socket file description to add to add to
      if (client_fd > 0)
        FD_SET(client_fd, &current_set);

      if (client_fd > highest_fd)
        highest_fd = client_fd;
    }

    sock_activity =
        select(highest_fd + 1, &current_set, nullptr, nullptr, nullptr);
    if (sock_activity < 0) {
      cerr << "Failed to get get socket acitiviy " << strerror(errno) << endl;
      /*continue;*/
    }

    // there is some activity happening on the server's socket
    if (FD_ISSET(server_fd, &current_set)) {

      /* it's either a new connection hence accept it*/
      if ((new_socket = accept(server_fd, (struct sockaddr *)&server_addr,
                               (socklen_t *)&sockaddr_len)) < 0) {
        handle_errors("Failed to accept new connection", server_fd);
      } else {
        // add new connection to cleints
        for (int i = 0; i < MAX_CLIENTS; i++) {

          // space for new client in the queue for clients
          if (clients[i] == 0) {
            clients[i] = new_socket;
            client_structs[i].fd = new_socket;
            cout << __LINE__ << ": Added new client to queue as socket "
                 << new_socket << endl;
            // inform server user of new connection and details

            break;
          }
        }
      }
    }
    /*or some I/O activity like writing to or reading from some other socket*/
    for (int i = 0; i < MAX_CLIENTS; i++) {
      client_fd = clients[i];
      if (FD_ISSET(client_fd, &current_set)) {
        //////////////////////////////////////////////////////////
        /////                                              ///////
        /////                                             ///////
        /////////////////////////////////////////////////////////
        // handle client disconneting from the server
        bytes_read = read(client_fd, buff, sizeof(buff));
        /*buff[length + 1] = '\0';*/
        if (bytes_read < 0) {
          cout << "Failed to read from incomming buffer " << bytes_read << endl;
        } else if (bytes_read == 0) {
          client_disconnect(client_fd, i);
        } else if (buff[0] == '!') {
          get_client_details(new_socket, i, buff);
          Client c = clients_map[new_socket]; // some reason with wont read
                                              // straight from the map
          broadcast_connection(new_socket, c.name, strlen(c.name));
        } else {
          broadcast_msg(client_fd, buff, bytes_read);
        }
      }
    }
  }
  return 0;
}

void handle_errors(const char *msg, int &arg) {
  cerr << msg << " " << strerror(errno) << "\n";
  close(arg);
  exit(EXIT_FAILURE);
}
void broadcast_msg(int sender, char *msg, size_t len) {
  int bytes_sent = 0;
  msg[len] = '\0';
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] != sender && clients[i] != 0) {
      char temp[MAX_BUFF];
      size_t fd_user_len = strlen(clients_map[sender].name);
      snprintf(temp, fd_user_len + strlen(msg) + 5, "[%s]: %s\n",
               clients_map[sender].name, msg);
      temp[strlen(temp)] = '\0';
      bytes_sent = send(clients[i], temp, strlen(temp), 0);
      if (bytes_sent < 0)
        cout << "Failed to broadcast messages " << strerror(errno) << endl;
    }
  }
}

void broadcast_connection(int new_client, char *msg, int name_len) {
  getpeername(client_fd, (struct sockaddr *)&server_addr,
              (socklen_t *)&sockaddr_len);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
  char send_buff[MAX_BUFF];
  /*std::string s = "++++++++" + msg + "++++++++\n";*/
  unsigned long n = snprintf(send_buff, sizeof(send_buff),
                             "++++++++%s joined chat++++++++\n%s\n", msg, ip);
  if (n >= sizeof(send_buff))
    cout << "Overflow could occur\n";
  else {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      // only announce to connected clients
      if (clients[i] != 0 && clients[i] != new_client) {
        send(clients[i], send_buff, strlen(send_buff), 0);
        continue;
      }
    }
  }
}

void client_disconnect(int fd, int i) {
  getpeername(fd, (struct sockaddr *)&server_addr, (socklen_t *)&sockaddr_len);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);

  cout << "Client Disconnected IP:" << ip << ":" << ntohs(server_addr.sin_port)
       << endl;
  clients[i] = 0; // removing client from lineup
  close(fd);
  FD_CLR(fd, &current_set); // remove socket from set
}

void get_client_details(int fd, int i, const char *username_buff) {
  // rigorous check purely for my sanity
  // implies that client at i and client struct at i == fd
  if (clients[i] == fd) {
    Client c;
    c.fd = fd;
    std::copy(username_buff + 1, username_buff + strlen(username_buff), c.name);
    clients_map[c.fd] = c;
    clients_map.insert_or_assign(c.fd, c);
    clients_map[fd] = c;
  }
  cout << clients_map[fd].name << endl;
}
