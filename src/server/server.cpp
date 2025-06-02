#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <print>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
using std::cout, std::cerr, std::endl;
using std::string;
constexpr int DEFAULT_PORT = 10000; // this is the default port if none is given
                                    // as a command-line arguments
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
int port = 0;
// holds all out active socker activity
fd_set master_set, current_set;
struct Client {
  int fd;
  char name[MAX_USR_LEN] = {'\0'};
  char password[MAX_USR_LEN]; // idk if i'll ever implement this
};

// containers
std::pair<int, std::string> clients[MAX_CLIENTS]; // K->fd,V->name
char buff[MAX_BUFF];                              // filled with newline values
std::unordered_map<std::string, Client> clients_map; // K->name,V->Client
void printClients();

// function signatures
void queue_client(int fd);
void printClientMap();
void handle_errors(const char *msg, int &arg);
void broadcast_msg(int sender, char *msg, size_t len);
void broadcast_connection(int new_client, const char *msg);
void get_client_details(int fd, int i, const char *username_buff);
void client_disconnect(int fd, int index);
int sanitize_port_number(int port_number);

int main(int argc, char *argv[]) {

  // allow for the server to run on a given port through command-line arguments
  if (argc != 2) {
    std::cerr << "Expected a port number and got nothing!\n";
    std::cout << "Using the default port " << DEFAULT_PORT << "\n";
  } else {
    port = sanitize_port_number(std::stoi(argv[1]));
  }

  // create the server socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    handle_errors("Failed to create server socket", errno);
  } else {
    cout << "Successfully made server socket" << endl;
  }

  // master server addrs
  server_addr = {AF_INET, htons(port), INADDR_ANY, {0}};
  sockaddr_len = sizeof(server_addr);
  // allows for socket to be reusable
  int opt = 1; // this option is only used for setting the server to be reusable
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) != 0) {
    handle_errors("Failed to make server reusable", server_fd);
  } else [[likely]] {
    cout << "Successfully made server reusable\n" << endl;
  }

  // bind the socket
  if (bind(server_fd, (const struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    handle_errors("Failed to bind socket to addr", server_fd);
  }

  // listen to connections
  if (listen(server_fd, 3) < 0) {
    handle_errors("Failed to listen to connection", server_fd);
  } else [[__likely__]] {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
    cout << "Server runnining on " << ip << ":" << ntohs(server_addr.sin_port)
         << "\n";
  }
  // have a fd set for allowing multiple clients
  FD_ZERO(&master_set);
  FD_SET(server_fd, &master_set); // server is always on top

  while (1) {
    // because the select function is destructive in that it will change/destroy
    // file descriptor sets everytime you use it
    current_set = master_set;
    highest_fd = server_fd;

    // add client (child) socket file descriptors into the set
    for (int i = 0; i < MAX_CLIENTS; i++) {
      client_fd = clients[i].first;
      /*clients[i].second = "";*/
      // looking for a valid client socket file description to add to add to
      if (client_fd > 0) {
        FD_SET(client_fd, &current_set);
      }

      if (client_fd > highest_fd) {
        highest_fd = client_fd;
      }
    }

    // Select can only read file descriptor less than 1024 (poll doesn't have
    // this weakness)
    sock_activity =
        select(highest_fd + 1, &current_set, nullptr, nullptr, nullptr);
    if (sock_activity < 0) {
      cerr << "Failed to get socket acitiviy " << strerror(errno) << endl;
    }

    // there is some activity happening on the server's socket
    if (FD_ISSET(server_fd, &current_set)) {

      /* it's either a new connection hence accept it*/
      if ((new_socket = accept(server_fd, (struct sockaddr *)&server_addr,
                               (socklen_t *)&sockaddr_len)) < 0) {
        handle_errors("Failed to accept new connection", server_fd);
      } else {
        queue_client(new_socket);
      }
    }
    /*or some I/O activity like writing to or reading from some other socket*/
    for (int i = 0; i < MAX_CLIENTS; i++) {
      client_fd = clients[i].first;
      if (FD_ISSET(client_fd, &current_set)) {
        // handle client disconneting from the server
        bytes_read = read(client_fd, buff, sizeof(buff));
        /*buff[length + 1] = '\0';*/
        if (bytes_read < 0) {
          cout << "Failed to read from incomming buffer " << bytes_read << endl;
        } else if (bytes_read == 0) {
          client_disconnect(client_fd, i);
        }
        /*Protocol for when a new client joins chat*/
        else if (buff[0] == '!') {
          get_client_details(new_socket, i, buff);
          broadcast_connection(new_socket, clients[i].second.c_str());
        } else {
          broadcast_msg(client_fd, buff, bytes_read);
        }
      }
    }
  }
  return 0;
}

/**
 * @brief handles and kind of socket error, prints out corresponding errors from
 * errno given by the code that calls it and closes the socket
 * @param msg is char array of any error message you'd like to log
 * @param arg error code used to exit
 */
void handle_errors(const char *msg, int &arg) {
  cerr << msg << " " << strerror(errno) << "\n";
  close(arg);
  exit(EXIT_FAILURE);
}

/**
 * @brief broadcasts received data from one client to the rest of the other
 * connected clients
 * @param sender client file descriptors whose received data will be broadcasted
 * @param msg recieved data
 * @len length of the received data
 */
void broadcast_msg(int sender, char *msg, size_t len) {
  int bytes_sent = 0;
  msg[len] = '\0';
  for (int i = 0; i < MAX_CLIENTS; i++) {

    if (clients[i].first != sender && clients[i].first != 0) {
      char temp[MAX_BUFF];
      size_t fd_user_len = (clients[i].second.length());

      snprintf(temp, fd_user_len + strlen(msg) + 5, "[%s]: %s\n",
               clients[i].second.c_str(), msg);

      cout << __LINE__ << ": temp " << temp << '\n'
           << ": fd_user_len " << fd_user_len << '\n'
           << "msg " << msg << endl;

      temp[strlen(temp)] = '\0';
      bytes_sent = send(clients[i].first, temp, strlen(temp), 0);
      if (bytes_sent < 0)
        cout << "Failed to broadcast messages " << strerror(errno) << endl;
    }
  }
}

/**
 * @brief broadcasts to the connected client that a new client has joined the
 * chat
 * @param new_client the newest connected client on the server
 * @param msg message the server will broadcast to connected client
 */
void broadcast_connection(int new_client, const char *msg) {

  getpeername(client_fd, (struct sockaddr *)&server_addr,
              (socklen_t *)&sockaddr_len);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);
  char send_buff[MAX_BUFF];
  for (int i = 0; i < MAX_BUFF; i++) {
    send_buff[i] = '\0';
  }
  unsigned long n = snprintf(send_buff, sizeof(send_buff),
                             "++++++++%s joined chat++++++++\n", msg);
  if (n >= sizeof(send_buff)) {
    cout << "Overflow could occur\n";
  } else {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      // only announce to connected clients
      if (clients[i].first != 0 && clients[i].first != new_client) {
        send(clients[i].first, send_buff, strlen(send_buff), 0);
        continue;
      }
    }
  }
}

/**
 * @brief writes to the server user what client has Disconnected
 * @param fd disconnected client
 * @param index of disconnected client in the client queue
 */
void client_disconnect(int fd, int index) {
  getpeername(fd, (struct sockaddr *)&server_addr, (socklen_t *)&sockaddr_len);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(server_addr.sin_addr), ip, INET_ADDRSTRLEN);

  cout << "Client Disconnected IP:" << ip << ":" << ntohs(server_addr.sin_port)
       << endl;
  clients[index].first = 0; // removing client from lineup
  clients[index].second = "";
  // Reorder the clients queue
  //*Cursor should only exist for valid connectinos */
  int temp_count = client_count;

  for (int i = 0; i < client_count; i++) {
    // first find the removed client
    if (clients[i].first != 0) // client is marked as disconnected
      continue;
    // Shift every client on it's right to it's left
    for (int j = i; j <= client_count; j++) {
      // when the code gets here, the first index pointed too will have a
      // disconnected client at that index
      if (clients[j + 1].first != 0) {
        clients[j].first =
            clients[j + 1].first; // move valid connections to the left
        clients[j + 1].first = 0; // move the 0s to the right
        clients[j + 1].second = "";
        break;
      }
    }
  }

  // i expect this function to be called everytime a client disconnects
  // thus we decrement the client_count once every function call
  temp_count--;

  client_count = temp_count;
  cout << "Disconnected List\n";
  printClients();
  close(fd);
  FD_CLR(fd, &current_set); // remove socket from set
}

/**
 * @brief adds the client details from client to the server
 * @param fd file descriptor of the client whose details we're saving
 * @param username_buff client user name with delimeter
 */
void get_client_details(int fd, int i, const char *username_buff) {
  if (clients[i].first == fd) {
    // should now have an array of fd and usernames
    string username(username_buff + 1, username_buff + strlen(username_buff));
    clients[i].second = std::move(username);
    cout << __PRETTY_FUNCTION__ << ": " << clients[i].second << endl;
    cout << "username after move " << username << endl;

    // Add these to our map
    clients_map[username].fd = fd;
    std::copy(username_buff + 1, username_buff + strlen(username_buff),
              clients_map[username].name);
  }
}

/**
 * @brief sanitises port number to be between 1024 and 65k
 * @param port is the port number provided via command-line arguments
 */
int sanitize_port_number(int port_number) {
  if (port_number < 1024) {
    std::cerr << "Cannot Use Reservered Port Number\nReverting to using "
                 "default server port :"
              << DEFAULT_PORT << '\n';
    return DEFAULT_PORT;
  } else if (port_number > 65535) {
    std::cerr << "Cannot Use port number greater than " << (65535)
              << " wtf is wrong with you\n";
    return DEFAULT_PORT;
  }
  port = port_number;
  return port;
}
/**
 * @brief adds a new client to the client %dqueue
 * @param fd the client  file discriptor to be enqueued
 */
void queue_client(int fd) {

  if (client_count > MAX_CLIENTS) {
    cerr << "CLIENT QUEUE IS FULL\n";
    return;
  }

  // because we keep track of how many clients are connected
  // we can use that as an index into the client queue instead of
  // going from the beginning everytime
  for (int i = client_count; i < MAX_CLIENTS; i++) {
    if (clients[i].first == 0) {
      clients[i].first = fd;
      /*clients_map[i].name=*/
      client_count++;
      /*std::cout << __FUNCTION__ << ":" << __LINE__ << " Client Count "*/
      /*<< client_count << " FD: " << fd << endl;*/
      break;
    }
  }
}

void printClients() {
  for (int i = 0; i < client_count; i++) {
    cout << "Fd from array -> " << clients[i].first << "\n";
    cout << "Name from array ->" << clients[i].second.c_str() << "\n";
  }
  cout << '\n';
}
void printClientMap() {
  for (auto &[K, V] : clients_map) {
    std::cout << "Key ->" << K << " Value {" << V.fd << "," << V.name << "}"
              << '\n';
  }
  cout << endl;
}
