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
void disconnect();

////these will run on their own seperate threads
void *send_msg(void *fd);
void *receive_msg(void *fd);
////////////////////////////////////////////////

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

/**
 *@brief prints out a help menu to the cmd when incorrect number of arguments
 *are given to the program
 @param argc is the amount of commmand-line arguments passsed into the program
 */
void help(int argc) {
  cerr << "Incorrect amount of command line arguments " << argc
       << "expected 3\n";
  cout << "<user name>=32 characters  <ip> <port>\n";
  exit(EXIT_FAILURE);
}

/**
 *@brief writes the client information (such as user )to the server once the
 *3-way TCP handshake is done and a connection is established
 */
void write_client_info() {
  std::string name(NAME_LEN + 2, '\0');
  name[0] = '!';
  name.insert(1, user_name);
  if (send(socket_fd, name.c_str(), name.length(), 0) < 0) {
    cerr << "failed to send user name for server\n";
  }

  // Receiving a welcome from the server
  char recv_buff[1024] = {'\0'};
  if (read(socket_fd, recv_buff, sizeof(recv_buff)) < 0) {
    cerr << "failed to read welcome message from chat server\n";
  }
  cout << recv_buff; // comes with a '\0' attached
}

/**
 *@brief handles any error that may occur when trying to connect to the server.
 * If an issue occurs before a connection is successfully made the client exits
 *@param msg the error message you wish to log before the client exits
 *@param arg the file descriptor to exit from
 */
void handle_errors(const char *msg, int &arg) {
  cerr << msg << " " << errno << " " << strerror(errno) << endl;
  close(arg);
  exit(EXIT_FAILURE);
}

/**
 *@brief sends the messages from the client to the the server. This function
 *runs in it's own thread
 *@param fd is the file descriptor of the client once connected to the server
 */
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

/**
 *@brief receives the messages from the server that were sent by other clients.
 *This function runs in it's own thread
 *@param fd is the file descriptor of the client once connected to the serv*/
void *receive_msg(void *fd) {
  int client_fd = *((int *)fd);

  char recieve_buff[MSG_LEN + NAME_LEN];
  while (1) {

    int bytes_in = read(client_fd, recieve_buff, sizeof(recieve_buff));
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
    /*std::string recieve_str(recieve_buff, bytes_in);*/
    cout << recieve_buff << endl;
    /*cout << "::type ";*/
    std::memset(recieve_buff, '\0',
                sizeof(recieve_buff)); // this is supposed to just set
                                       // everything in the buffer to 'nothing'
  }
}

/**
 *@brief this disconnects the client from the server and closes the client
 *socket
 */
void disconnect() {
  close(socket_fd);
  exit(0);
}
