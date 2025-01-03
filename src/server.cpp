#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

bool is_running = false;
int server_socket_fd = -1; // initial state
void handle_close_signal(int sig);

int main() {
  // Create a socket  with IPv4
  signal(SIGINT, handle_close_signal);
  server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket_fd < 0) {
    std::cerr << "Failed To create socket\n";
    close(server_socket_fd);
  } else {
    std::cout << "Successfully created server socket\n";
    is_running = true;
    std::cout << "Running status " << is_running << std::endl;
  }
  // Bind to that socket using any port above 1023 and localhost
  sockaddr_in address{AF_INET, htons(9999), {0}};
  int bind_stat = bind(server_socket_fd, (const struct sockaddr *)&address,
                       sizeof(address));
  if (bind_stat < 0) {
    std::cerr << "Failed To bind socket " << strerror(errno) << " \n";
    close(server_socket_fd);
  } else if (bind_stat == 0) {
    std::cout << "Binded " << ntohs(address.sin_port) << strerror(errno)
              << std::endl;
  }
  // Listen to the incomming connection from that socket
  int listen_stat = listen(server_socket_fd, 3);
  if (listen_stat < 0) {
    std::cerr << "Failed to listen to socket connections\n";
    close(bind_stat);
    close(server_socket_fd);
  } else if (listen_stat == 0) {
    std::cout << "Listening for client on port " << ntohs(address.sin_port)
              << std::endl;
  }
  // accept a client into the socket
  int client = accept(server_socket_fd, 0, 0);
  // poll for listen events from the client and server side
  if (client < 0) {
    std::cerr << "Failed to accept client connection " << strerror(errno)
              << std::endl;
  } else {
    std::cout << "Connection accepted\n";
  }
  struct pollfd polled_fds[2] = {{0,       // stdin == 0 fd
                                  POLL_IN, // there is data to be read
                                  0},
                                 {client,
                                  POLL_IN, // there is data to be read
                                  0}};
  std::cout << "Done Polling\nAbout to execute chat\n";
  while (is_running) {
    // Buffer to hold messages to be sent bach and forth
    char buff[1024];
    /*std::cout << "while loop\n";*/
    int polled_result = poll(
        polled_fds, 2, 50000); // poll for 5seconds. It'll block for 5
                               // seconds or untl a file descriptor is ready
    if (polled_result < 0) {
      std::cerr << "Server failed to poll connections" << strerror(errno)
                << std::endl;
    }
    if (polled_result == 0) {
      /*std::cerr << "Server timed out\n";*/
    }

    // Execute the chat
    if (polled_fds[0].revents & POLL_IN) {
      // read what the client sent
      read(0, buff, sizeof(buff) - 1);
      /*std::cout << "Server Reading " << read_stat << "\n";*/
      // allows the server to write back to the client
      send(client, buff, sizeof(buff) - 1, 0);
    } else if (polled_fds[1].revents & POLL_IN) {
      // reason for the -1 is to explicity leave space for EOF(string
      // terminator)
      if (recv(client, buff, sizeof(buff) - 1, 0) == 0) {
        std::cout << "Server received no data in buffer " << strerror(errno)
                  << std::endl;
      }
      std::cout << "Client: " << buff << '\n'; // show the text recieved text
    }
  }

  return 0;
}

void handle_close_signal(int sig) {
  std::cout << "Recieved close signal " << sig << " \n";
  if (server_socket_fd != -1) {
    std::cout << "Closing working server \n";
    close(server_socket_fd);
  } else if (server_socket_fd < 0) {
    std::cout << "Closing dead server\n";
    close(server_socket_fd);
  }
  std::cout << "Exiting server\n";
  exit(0);
};
