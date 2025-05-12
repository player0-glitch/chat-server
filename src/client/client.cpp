#include <algorithm>
#include <arpa/inet.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <string.h> //strlen
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using std::cout, std::endl, std::cerr;

// Constants
constexpr int NAME_LEN = 32;
constexpr int MSG_LEN = 1024;

// globals
int socket_fd = -1;
sockaddr_in address;
unsigned int port = 0;
char ip[INET_ADDRSTRLEN]; // 16 -> length of an ip address
pthread_t send_thread;
pthread_t receive_thread;

// containers
std::string user_name(NAME_LEN, '\0');
char buff[MSG_LEN + NAME_LEN];
char send_buff[MSG_LEN + NAME_LEN];

// function signature
void help(int argc);
void handle_errors(const char *msg, int &arg);
void write_client_info();
void *send_msg(void *fd);
void *receive_msg(void *fd);
void disconnect();
int main(int argc, char *argv[]) {

  // argc is the number of command line arguments
  if (argc != 4) {
    help(argc);
  }
  // Setting up the client information
  user_name = argv[1]; //  the first arguments is the name of the program
  std::copy(argv[2], argv[2] + strlen(argv[2]), ip);
  port = std::stoi(argv[3]);

  // Create the client socket
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    handle_errors("Failed to create a client socket", socket_fd);
  }

  address.sin_port = htons(port);
  address.sin_family = AF_INET;
  if (inet_pton(AF_INET, ip, &address.sin_addr) < 0) {
    handle_errors("IP Address error ", socket_fd);
  }

  // Connect client
  if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    handle_errors("Failed to connect client to server", socket_fd);
  }

  write_client_info();

  pthread_create(&send_thread, nullptr, send_msg, (void *)&socket_fd);
  pthread_create(&receive_thread, nullptr, receive_msg, (void *)&socket_fd);

  pthread_join(send_thread, nullptr);
  pthread_join(receive_thread, nullptr);

  close(socket_fd); // gracefully close the socket
  return 0;
}
void help(int argc) {
  cerr << "Incorrect amount of command line arguments " << argc
       << "expected 3\n";
  cout << "<user name>=32 <ip> <port>\n";
  exit(EXIT_FAILURE);
}
void write_client_info() {
  std::string name(NAME_LEN + 2, '\0');
  name[0] = '!';
  name.insert(1, user_name);
  if (send(socket_fd, name.c_str(), name.length(), 0) < 0) {
    cerr << "failed to send user name for server\n";
  }
}

void handle_errors(const char *msg, int &arg) {
  cerr << msg << " " << errno << " " << strerror(errno) << endl;
  close(arg);
  exit(EXIT_FAILURE);
}

void *send_msg(void *fd) {
  int client_fd = *((int *)fd);
  std::string message(MSG_LEN, '\0');
  std::string dettach = "!q";
  while (1) {
    // get lient input
    std::getline(std::cin, message);

    // check for if the user wants to disconnect
    if (message.find(dettach) != std::string::npos) {
      disconnect();
      return nullptr;
    }

    ssize_t sent_bytes = send(client_fd, message.c_str(), message.length(), 0);
    if (sent_bytes < 0) {
      cerr << "Failed to send data from terminal to server\n";
    }
  }
  return nullptr;
}

void *receive_msg(void *fd) {
  int client_fd = *((int *)fd);
  while (1) {

    char recieve_buff[MSG_LEN + NAME_LEN];

    int bytes_in = read(client_fd, recieve_buff, sizeof(recieve_buff));
    /*cout << "::type ";*/
    if (bytes_in < 0) {
      cerr << "Failed to read from the server\n";
    } else if (bytes_in == 0) {
      cerr << "Server issue. Dettaching!\n";

      // code might not even reach here.
      // only God knows how long I've been at this so I'll just dettach
      // the thread from within here.
      if (pthread_detach(pthread_self()) != 0) {
        cerr << "Failed to detach itself\n";
      } else {
        cout << "Client Thread should have detached itself\n";
        pthread_exit(nullptr); // should do what #147 is supposed
      }
      return nullptr;
    }
    std::string recieve_str(recieve_buff, bytes_in);
    cout << recieve_buff << endl;
    std::memset(recieve_buff, '\0', sizeof(recieve_buff));
  }
}

void disconnect() {
  close(socket_fd);
  exit(0);
}
