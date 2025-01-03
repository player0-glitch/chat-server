#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

void handle_close_signal(int sig);
int client_socket_fd = -1;
bool is_connected = false;
int main() {
  // Gracefull handling of client Closing
  signal(SIGINT, handle_close_signal);

  // Create a socket  with IPv4
  client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (client_socket_fd < 0) {
    std::cerr << "Failed to create socket " << strerror(errno) << std::endl;
    close(client_socket_fd);
  } else if (client_socket_fd == 0) {
    is_connected = true;
    std::cout << "Successfully created client socket " << is_connected
              << std::endl;
  }
  // Bind to that socket using any port above 1023 and localhost
  sockaddr_in address{AF_INET, htons(9999), {0}};

  int connection_stat = connect(
      client_socket_fd, (const struct sockaddr *)&address, sizeof(address));
  if (connection_stat < 0) {
    std::cerr << "Client failed to connect to server " << strerror(errno)
              << std::endl;
    close(client_socket_fd);
  } else {
    is_connected = true;
    std::cout << "Successfully connected client to server " << is_connected
              << " " << strerror(errno) << std::endl;
  }

  /*int server = accept(client_socket_fd, 0, 0);*/
  // poll for listen events from the client and server side

  struct pollfd polled_fds[2] = {{0,       // stdin == 0 fd
                                  POLL_IN, // there is data to be read
                                  0},
                                 {client_socket_fd,
                                  POLL_IN, // there is data to be read
                                  0}};
  std::cout << "Done Polling\nAbout To execute chat\n" << is_connected << "\n";
  while (is_connected) {
    // Buffer to hold messages to be sent bach and forth
    char buff[1024];
    /*std::cout << "While\n";*/
    int polled_result = poll(
        polled_fds, 2, 50000); // poll for 5seconds. It'll block for 5
                               // seconds or untl a file descriptor is ready
    if (polled_result < 0) {
      std::cerr << "Server failed to poll connections\n";
      close(client_socket_fd);
    }
    if (polled_result == 0) {
      std::cerr << "Server timed out\n";
    }

    // Execute the chat
    if (polled_fds[0].revents & POLL_IN) {
      // read what the client sent
      read(0, buff, sizeof(buff) - 1);
      // allows the server to write back to the client
      /*std::cout << "Client Reading " << read_stat << '\n';*/
      send(client_socket_fd, buff, sizeof(buff) - 1, 0);
    } else if (polled_fds[1].revents & POLL_IN) {
      // reason for the -1 is to explicity leave space for EOF(string
      // terminator)

      if (recv(client_socket_fd, buff, sizeof(buff) - 1, 0) == 0) {
        std::cout << "Client got no data recieved from buffer "
                  << strerror(errno) << std::endl;
      }
      std::cout << "Server: " << buff << '\n'; // show the text recieved text
    }
  }
  return 0;
}
void handle_close_signal(int sig) {
  std::cout << "Recieved close signal " << sig << " \n";
  if (client_socket_fd != -1) {
    std::cout << "Closing server \n";
    close(client_socket_fd);
  }
  std::cout << "Exiting client\n";
  exit(0);
};
