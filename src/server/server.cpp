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
constexpr int MAX_CLIENTS = 256;
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
int client_count = 0;

// holds all out active socker activity
fd_set master_set, current_set;
struct Client {
  int fd;
  char name[MAX_USR_LEN] = {'\0'};
  char password[MAX_USR_LEN]; // idk if i'll ever implement this
};

// containers
int clients[MAX_CLIENTS];
char buff[MAX_BUFF]; // filled with newline values
std::unordered_map<int, Client> clients_map;

// function signatures
void queue_client(int fd);
void handle_errors(const char *msg, int &arg);
void broadcast_msg(int sender, char *msg, size_t len);
void broadcast_connection(int new_client, char *msg);
void get_client_details(int fd, int i, const char *username_buff);
void client_disconnect(int fd, int i);

int main() {
  // create the server socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    handle_errors("Failed to create server socket", errno);
  } else {
    cout << "Successfully made server socket" << endl;
  }

  // master server addrs
  server_addr = {AF_INET, htons(PORT), INADDR_ANY, {0}};
  sockaddr_len = sizeof(server_addr);
  // allows for socket to be reusable
  int opt = 1; // this option is only used for setting the server to be reusable
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) != 0) {
    handle_errors("Failed to make server reusable", server_fd);
  } else {
    cout << "Successfully made server reusable" << endl;
  }

  // bind the socket
  if (bind(server_fd, (const struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    handle_errors("Failed to bind socket to addr", server_fd);
  } else {
    cout << "Successfully binded socket to addr" << endl;
  }

  // listen to connections
  if (listen(server_fd, 3) < 0) {
    handle_errors("Failed to listen to connection", server_fd);
  } else {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
    cout << "Server runnining on " << ip << ":" << ntohs(server_addr.sin_port)
         << "\n";
  }
  // have a fd set for allowing multiple clients
  FD_ZERO(&master_set);
  FD_SET(server_fd, &master_set); // server is always on top
                                  //

  while (1) {
    // because the select function is destructive in that it will change/destroy
    // file descriptor sets everytime you use it
    current_set = master_set;
    highest_fd = server_fd;

    // add client (child) socket file descriptors into the set
    for (int i = 0; i < MAX_CLIENTS; i++) {
      client_fd = clients[i];
      // looking for a valid client socket file description to add to add to
      if (client_fd > 0) {
        FD_SET(client_fd, &current_set);
      }

      if (client_fd > highest_fd) {
        highest_fd = client_fd;
      }
    }

    sock_activity =
        select(highest_fd + 1, &current_set, nullptr, nullptr, nullptr);
    if (sock_activity < 0) {
      cerr << "Failed to get get socket acitiviy " << strerror(errno) << endl;
    }

    // there is some activity happening on the server's socket
    if (FD_ISSET(server_fd, &current_set)) {

      /* it's either a new connection hence accept it*/
      if ((new_socket = accept(server_fd, (struct sockaddr *)&server_addr,
                               (socklen_t *)&sockaddr_len)) < 0) {
        handle_errors("Failed to accept new connection", server_fd);
      } else {
        /*{
          Timer timer;
          // add new connection to cleints
          for (int i = 0; i < MAX_CLIENTS; i++) {

            // space for new client in the queue for clients
            if (clients[i] == 0) {
              clients[i] = new_socket;
              cout << __LINE__ << ": Added new client to queue as socket "
                   << new_socket << "\n";
              // inform server user of new connection and details

              break;
            }
          }

        }*/
        // this does the same thing as the uncommented code but has a more
        // consistent performance (smaller standard deviation)
        queue_client(new_socket);
      }
    }
    /*or some I/O activity like writing to or reading from some other socket*/
    for (int i = 0; i < MAX_CLIENTS; i++) {
      client_fd = clients[i];
      if (FD_ISSET(client_fd, &current_set)) {
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
          broadcast_connection(new_socket, c.name);
        } else {
          broadcast_msg(client_fd, buff, bytes_read);
        }
      }
    }
  }
  return 0;
}

/* @brief handles and kind of socket error, prints out corresponding errors from
 * errno given by the code that calls it and closes the socket
 */
void handle_errors(const char *msg, int &arg) {
  cerr << msg << " " << strerror(errno) << "\n";
  close(arg);
  exit(EXIT_FAILURE);
}

/* @brief broadcasts received data from one client to the rest of the other
 * connected clients
 * @param sender client file descriptors whose received data will be broadcasted
 * @ param msg recieved data
 * @ len length of the received data
 */
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

/* @brief broadcasts to the connected client that a new client has joined the
 * chat
 * @param new_client the newest connected client on the server
 * @msg message the server will broadcast to connected client
 */
void broadcast_connection(int new_client, char *msg) {
  getpeername(client_fd, (struct sockaddr *)&server_addr,
              (socklen_t *)&sockaddr_len);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
  char send_buff[MAX_BUFF];
  cout << "Broadcast Connection buffer check\n";
  for (int i = 0; i < MAX_BUFF; i++) {
    send_buff[i] = '\0';
  }
  unsigned long n = snprintf(send_buff, sizeof(send_buff),
                             "++++++++%s joined chat++++++++\n%s\n", msg, ip);
  if (n >= sizeof(send_buff)) {
    cout << "Overflow could occur\n";
  } else {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      // only announce to connected clients
      if (clients[i] != 0 && clients[i] != new_client) {
        send(clients[i], send_buff, strlen(send_buff), 0);
        continue;
      }
    }
  }
}

/* @brief writes to the server user what client has Disconnected
 * @param fd disconnected client
 * @param index of disconnected client in the client queue
 */
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

/* @brief adds the client details from client to the server
 * @param fd file descriptor of the client whose details we're saving
 * @username_buff client user name with delimeter
 */
void get_client_details(int fd, int i, const char *username_buff) {
  if (clients[i] == fd) {
    Client c;
    c.fd = fd;
    std::copy(username_buff + 1, username_buff + strlen(username_buff), c.name);
    clients_map[fd] = c;
  }
  cout << clients_map[fd].name << endl;
}

/* @brief adds a new client to the client queue
 * @fd the client  file discriptor to be enqueued
 */
void queue_client(int fd) {

  if (client_count > MAX_CLIENTS) {
    cerr << "CLIENT QUEUE IS FULL\n";
    return;
  }

  for (int i = client_count; i < MAX_CLIENTS; i++) {
    if (clients[i] == 0) {
      clients[i] = fd;
      client_count++;
      break;
    }
  }
}
