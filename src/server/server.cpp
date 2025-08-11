#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <print>
#include <string.h>
#include <string_view>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
// using std::cout, std::cerr, std::endl;
using std::pair, std::cerr;
using std::print, std::println;
using std::string, std::string_view;

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
pair<int, std::string> clients[MAX_CLIENTS]; // K->fd,V->name
char buff[MAX_BUFF] = {'\0'};                // filled with newline values
std::unordered_map<std::string, Client> clients_map; // K->name,V->Client

// function signatures
void queue_client(int fd);
void printClients();
void printClientMap();
void handle_errors(const char *msg, int &arg);
void broadcast_msg(int sender, char *msg, size_t len);
void broadcast_connection(int new_client, const char *msg);
void get_client_details(int fd, int i, const char *username_buff);
void client_disconnect(int fd, int index);
int sanitize_port_number(int port_number);
bool find_word_with_target(string_view target_symbol, string_view msg,
                           size_t &pos);
void send_DM(int sender_fd, std::string_view buf_view, size_t begin);
void trim_whitespace(string &str);
int find_sender_name(int sender_fd);

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
    println("Successfully made server socket");
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
    println("Successfully made server reusable");
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
    println("Server running on {}:{}", ip, ntohs(server_addr.sin_port));
  }
  // have a fd set for allowing multiple clients
  FD_ZERO(&master_set);
  FD_SET(server_fd, &master_set); // server is always on top
  const char *quit_symbol = "!q";
  const char *dm_symbol = "@";

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
      cerr << "Failed to get socket acitiviy " << strerror(errno) << std::endl;
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
        if (bytes_read < 0) {
          println("Failed to read from incoming buffer {}", bytes_read);
        } else if (bytes_read == 0) {
          client_disconnect(client_fd, i);
        }
        /*Protocol for when a new client joins chat*/
        else if (buff[0] == '!') {
          get_client_details(new_socket, i, buff);
          broadcast_connection(new_socket, clients[i].second.c_str());
        } else {
          /*Remove this branch. Be as branch-less as possible*/
          size_t pos = 0;
          if (find_word_with_target(dm_symbol, buff, pos)) {
            /*cout << __LINE__ << ": Pos Val-> " << pos << endl;*/
            send_DM(client_fd, buff, pos);
          } else if (find_word_with_target(quit_symbol, buff, pos)) {
            client_disconnect(client_fd, i);
          } else {
            broadcast_msg(client_fd, buff, bytes_read);
          }
        }
      }
      // clear out the buffer
      std::memset(buff, '\0', sizeof(buff));
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
  int pos = 0;
  int front = 0, back = client_count;
  for (int i = 0; i < client_count; i++) {
    // working with the front pointer
    if (clients[front].first != sender)
      front++;
    else if (clients[front].first == sender) {
      pos = front;
      break; // there's no need to keep search, we've found our user
    }

    // working with the back pointer
    if (clients[back].first != sender)
      back--;
    else if (clients[back].first == sender) {
      pos = back;
      break; // there's no need to keep search, we've found our user
    }
  }
  // DO the actual broadcasting
  for (int i = 0; i < MAX_CLIENTS; i++) {

    if (clients[i].first != sender && clients[i].first != 0) {

      char temp[MAX_BUFF];
      size_t fd_user_len = (clients[i].second.length());

      snprintf(temp, fd_user_len + strlen(msg) + 5, "[%s]: %s\n",
               clients[pos].second.c_str(), msg);

      temp[strlen(temp)] = '\0';
      bytes_sent = send(clients[i].first, temp, strlen(temp), 0);
      if (bytes_sent < 0)
        println("Failed to broadcast messages {}", strerror(errno));
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
    println("Buffer Overflow could happen over here");
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

  println("Client Disconnected IP {}:{}", ip, ntohs(server_addr.sin_port));

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
        // move valid connections to the left
        clients[j].first = clients[j + 1].first;
        clients[j].second = clients[j + 1].second;
        clients[j + 1].first = 0; // move the zero-ed out client to the right
        clients[j + 1].second = "";
        break;
      }
    }
  }

  // i expect this function to be called everytime a client disconnects
  // thus we decrement the client_count once every function call
  temp_count--;

  client_count = temp_count;
  println("Disconnected client ");
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
  using std::copy;
  if (clients[i].first == fd) {
    // should now have an array of fd and usernames
    string username(username_buff + 1, username_buff + strlen(username_buff));
    trim_whitespace(username);
    // Add these to our map
    clients_map[username].fd = fd;
    copy(username_buff + 1, username_buff + strlen(username_buff),
         clients_map[username].name);

    // Send welcome message to the newly connected client
    string welcome_msg = "Welcome to the Chat " + username + "!\n";

    if (send(fd, welcome_msg.c_str(),
             strlen(welcome_msg.c_str()) + strlen(username.c_str()),

             0) < 0) {
      cerr << "Failed to send welcome message to newly connected client\n";
    }
    println("{}, has joined the chat", username);
    clients[i].second = std::move(username);
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
    println("Fd from array -> {}", clients[i].first);
    println("Name from array ->{}", clients[i].second);
  }
  println();
}
void printClientMap() {
  for (auto &[K, V] : clients_map) {
    std::println("Key -->{} Value [{},{}]", K, V.fd, V.name);
  }
  println();
}

/**
 * @brief the word with the given target_symbol
 * this method used to find the word with either '@' or '!q' which have defined
 * behaviour in this program
 * @param target_symbol is the symbol that will be targeted by the method
 * @param msg is the string that will be searched
 * @param pos this is only used for 'send_DM' method to make the search string
 * shorter
 */
bool find_word_with_target(string_view target_symbol, string_view msg,
                           size_t &pos) {

  // We will ignore the pos parameter when this function is called by the
  // client_disconnect function
  // However, for the send_dm feature, it'll give a starting point to
  // find the user being DM'd

  //  find the position of the target target_symbol
  auto at = msg.find(target_symbol);

  // npos is a property so it this context it should return index of our
  // found target
  if (at != string_view::npos) {
    /*cout << "found target_symbol at " << at << " which is [" << msg[at]*/
    /*<< "]\n";*/
    pos = at; // this will mark the beginning of the search
    std::println("Found Target as {}", target_symbol);
    return true;
  }
  return false;
}

void send_DM(int sender_fd, string_view str_view, size_t begin) {
  println("{}::", __PRETTY_FUNCTION__);
  size_t end = 0;
  int index = begin + 1;
  size_t str_length = MAX_USR_LEN;

  if (str_view.length() < MAX_USR_LEN)
    str_length = str_view.length();

  for (size_t i = index; i < str_length; i++) {
    println("{}", str_view.at(i));
    if (!std::isalnum(static_cast<unsigned char>(str_view.at(i))) &&
        !std::ispunct(static_cast<unsigned char>(str_view.at(i)))) {
      end = i; // if this is only assigned at the end of the loop then we've hit
      break; // we're only interested in the first whitespace after the username
    }
    // a null char and so the last index is the last character of the
    // username
  }

  if (end == 0) // for when the username is the last thing in the message
    end = str_view.length();

  int sender_pos = find_sender_name(sender_fd);
  println("Senderpos->{}", sender_pos);

  println("Begin+1-->{} \tEnd-->{}", (begin + 1), end);
  print("Length of string being viewed {}", str_view.length());

  string user((str_view.substr(begin + 1, end)));

  // make sure we dont have a 'collision' in finding the correct username
  auto c = clients_map.find(user);
  println("c->second-> (user found) {}", c->second.name);
  while (c->second.name == clients[sender_pos].second) {
    c = clients_map.find(user);
    println("Currently searching for the user to dm");
  }

  trim_whitespace(user);

  // This requires C++20 and above
  if (clients_map.contains(user)) {
    println("{}: Recepient found ", __LINE__);
    println("FD-->{}\nName -->{}", c->second.fd, c->second.name);
    println("{}: DM to send -->{}", __LINE__,
            str_view.substr(end, str_view.length()));
    auto create_DM = [&](string_view str_v) {
      // take the whole string and remove the @[username]
      string start = "";
      if (begin != 0) // otherwise you get an integer Overflow trust me
        start = str_v.substr(0, begin - 1);
      else
        start = "";
      println("{}: First part of DM->{}", __LINE__, start);
      string rest = str_v.substr(end, str_v.size()).data();
      println("{}: Last part of DM-->{}", __LINE__, rest);
      return start + rest;
    };

    string DM = create_DM(str_view);

    println("DM After processing -->{}", DM);
    char char_buff[MAX_BUFF]; //= {'\0'};

    println("Sender name -->{}\n Receiver name -->{}",
            clients[sender_pos].second, user);
    std::snprintf(
        char_buff,
        //(dm length+sender_name lenght+ string literal length) all as c strings
        strlen(DM.c_str()) + strlen(clients[sender_pos].second.c_str()) + 11,
        "[from: %s ]:%s", clients[sender_pos].second.c_str(), DM.c_str());
    println("CHAR_BUFF w dm--> {}", char_buff);

    // Arttempt to send the dm
    if (send(c->second.fd, char_buff, strlen(char_buff), 0) < 0) {
      std::cerr << "Failed to send DM\n";
    } else {
      println("{}:  Should have received the dm-->{}", c->second.name,
              char_buff);
    }

    std::memset(char_buff, '\0', sizeof(char_buff));
    user.erase();
  } else {
    std::cerr << "User does not exist in the room\n";
  }

  /*cout << "Fd to send to->" << c.fd << "\nRecepient->" << c.name << "\n";*/
}

int find_sender_name(int sender_fd) {
  using std::pair;
  int start = 0;
  int end = 0;

  // the chat is empty avoid this computation
  if (client_count != 0)
    end = client_count;

  println("{}:: Searching to find username", __PRETTY_FUNCTION__);
  for (int i = start; i < end; i++) {
    // using the forward pointer
    if (clients[start].first != sender_fd) {
      start++;
      println("Start {} \nStart Value-->{}", start, clients[start].first);
    } else {
      start = i;
      return start;
    }
    // using the backwards pointer
    if (clients[end - 1].first != sender_fd) {
      end--;
      println("End {} \n End Value-->{}", end, clients[end].first);
    } else {
      end = i;
      return end;
    }
  }
  return -1; // user was never part of the chat
};

void trim_whitespace(string &str) {
  using std::isspace;
  std::println("{}", __PRETTY_FUNCTION__);
  size_t front = 0;
  while (front < str.size() &&
         isspace(static_cast<unsigned char>(str.at(front)))) {
    front++; // trim from the front
  }
  size_t back = str.size();
  while (back > front &&
         isspace(static_cast<unsigned char>(str.at(back - 1)))) {
    back--; // trim from the back
  }
  if (back == front)
    return; // because wtf;
  str = str.substr(front, back - front);
  std::println("TRIM --> {}", str);
  return;
}
