#include "../include/select_server.h"

User user_table[30];

// user -------------------------------------------------------------------------
int add_user(char* ip, char* port, int new_fd){
    int new_user_id;
    
    // find the smallest unused id
    std::vector <int> used_id;
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id != -1){
            used_id.push_back(user_table[i].id);
        }
    }
    
    for (size_t i = 1; i < MAX_USER_NUM; i++){
        // if i not in used_id
        if (std::find(used_id.begin(), used_id.end(), i) == used_id.end()){
            new_user_id = i;
            break;
        }
    }

    // find an empty entry
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id == -1){
            user_table[i].id = new_user_id;
            strcpy(user_table[i].ip, ip);
            strcpy(user_table[i].name, "(no name)");
            strcpy(user_table[i].port, port);
            user_table[i].fd = new_fd;
            break;
        }
    }

    return new_user_id;
}

void init_user_table(){
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        user_table[i].id = -1;
        user_table[i].clear = true;
    }
}

void remove_user(int id){
  for (size_t i = 0; i < MAX_USER_NUM; i++){
        // std::cout << "user[i].id = " << user_table[i].id << " id = " << id << std::endl;
        if (user_table[i].id == id){
            user_table[i].id = -1;
            break;
        }
    }
}

User get_user_by_id(int id){
  for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (id == user_table[i].id){
            return user_table[i];
        }
    }
    // not exist
    User u;
    u.id = -1;
    strcpy(u.name, "not_exist");
    return u;
}

User get_user_by_fd(int fd){
  for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (fd == user_table[i].fd){
            return user_table[i];
        }
    }
    // not exist
    User u;
    u.id = -1;
    strcpy(u.name, "not_exist");
    return u;
}

void who(int id, int client_fd){
  std::string header = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
  send(client_fd, header.c_str(), header.length(), 0);

  for (size_t i = 0; i < MAX_USER_NUM; i++){
    if (user_table[i].id != -1){
      std::stringstream ss;
      ss << user_table[i].id << "\t" 
              << user_table[i].name << "\t"
              << user_table[i].ip << ":"
              << user_table[i].port;
      if (user_table[i].id == id){
        ss << "\t<-me";
      }
      ss << "\n";
      send(client_fd, ss.str().c_str(), ss.str().length(), 0);
    }
  }
}

void name(int id, std::string user_name){
  // check if name exist
  for (size_t i = 0; i < MAX_USER_NUM; i++){
    if (user_table[i].id != -1){
      if (strcmp(user_name.c_str(), user_table[i].name) == 0){
        std::stringstream ss;
        ss << "*** User '" << user_name << "' already exists. ***\n";
        send(get_user_by_id(id).fd, ss.str().c_str(), ss.str().length(), 0);
        return;
      }
    }
  }

  // update user name
  for (size_t i = 0; i < MAX_USER_NUM; i++){
    if (user_table[i].id == id){
      strcpy(user_table[i].name, user_name.c_str());
      std::string name_msg = 
      "*** User from " + std::string(user_table[i].ip) + ":" + \
      std::string(user_table[i].port) + " is named '" + name + "'. ***\n";
      broadcast(name_msg);
      break;
    }
  }
}

void yell(int id, std::string msg){
  std::string yell_msg = 
      "*** " + std::string(get_user_by_id(id).name) + " yelled ***: " + msg + "\n";
  broadcast(yell_msg);
}

void tell(int from, int to, std::string msg){
  send(get_user_by_id(to).fd, msg.c_str(), msg.length(), 0);
}


// connect

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void broadcast(std::string msg){
  for (size_t i = 0; i < MAX_USER_NUM; i++){
    if (user_table[i].id != -1){
      send(user_table[i].fd, msg.c_str(), msg.length(), 0);
    }
  }
}

int main(int argc, char* argv[])
{
  // server
  fd_set server_fd;
  // fd of select
  fd_set client_fds;
  int fdmax;

  init_user_table();

  int listener; // fd of listening socket
  int new_fd; // accept() socket descriptor
  struct sockaddr_storage client_addr;
  socklen_t addrlen;

  char client_input[MAX_INPUT_LENGTH];
  int nbytes;

  char remoteIP[INET6_ADDRSTRLEN];

  int yes=1; // for SO_REUSEADDR
  int i, j, rv;

  struct addrinfo hints, *ai, *p;

  FD_ZERO(&server_fd);
  FD_ZERO(&client_fds);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int port = std::stoi(argv[1]);
  char* PORT = argv[1];

  if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
    fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
    exit(1);
  }

  for(p = ai; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0) {
      continue;
    }

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
      close(listener);
      continue;
    }

    break;
  }

  // bind failed
  if (p == NULL) {
    fprintf(stderr, "select_server: failed to bind\n");
    exit(2);
  }

  // free addrinfo
  freeaddrinfo(ai);

  // listen
  if (listen(listener, 10) == -1) {
    perror("listen");
    exit(3);
  }

  // add listener to the server_fd
  FD_SET(listener, &server_fd);

  // keep track on the max fd
  fdmax = listener;
  std::cout << "wait for connection ... " << std::endl;

  // main loop
  while(true) {
    client_fds = server_fd;

    if (select(fdmax+1, &client_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    // find updated data in the current connections
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &client_fds)) { // found
        if (i == listener) {
          // handle new connections
          socklen_t addr_size = sizeof client_addr;
          new_fd = accept(listener, (struct sockaddr *)&client_addr, &addr_size);

          if (new_fd == -1) {
            perror("accept");
          } 
          // new user connect
          else {
            
            // get client ip and port
            char ip_str[NI_MAXHOST];
            char port_str[NI_MAXSERV];

            if (getnameinfo((struct sockaddr *)&client_addr, 
                addr_size, ip_str, sizeof(ip_str), port_str, sizeof(port_str), 
                NI_NUMERICHOST | NI_NUMERICSERV) == 0){
                printf("select_server: got connection from %s:%s\n", ip_str, port_str);
                add_user(ip_str, port_str, new_fd);
            }

            // add new client to server_fd
            FD_SET(new_fd, &server_fd);

            // keep track of the max fd
            if (new_fd > fdmax) {
              fdmax = new_fd;
            }

            // show welcome msg
            std::string welcome_msg = 
            "****************************************\n"
            "** Welcome to the information server. **\n"
            "****************************************\n";
            send(new_fd, welcome_msg.c_str(), welcome_msg.size(), 0);

            // broadcast
            std::string login_msg;
            login_msg = "*** User '(no name)' entered from " + std::string(ip_str) + \
                        ":" + std::string(port_str) + ". ***\n";
            broadcast(login_msg);
            // first prompt
            send(new_fd, "% ", 3, 0);
          }
        } 

        // client main
        else {
            // connection close
            if ((nbytes = recv(i, client_input, sizeof client_input, 0)) <= 0) {
            // got error or connection closed by client
            if (nbytes == 0) {
              printf("selectserver: socket %d left up\n", i);
              std::string left_msg = \
                "*** User '" + std::string(get_user_by_fd(i).name) + "' left. ***\n";
              broadcast(left_msg);
            } else {
              perror("recv");
            }
            // close connection
            close(i);
            // remove from server fd list
            FD_CLR(i, &server_fd);
            remove_user(get_user_by_fd(i).id);
          }
          // get input from user
          else{
            User u = get_user_by_fd(i);
            who(u.id, u.fd);
            // pid_t pid = fork();
            // if (pid == 0){
            //     dup2(i, STDOUT_FILENO);
            //     close(i);
            //     User u = get_user_by_fd(i);
                
            //     // execlp("bin/ls", "bin/ls", (char*) NULL);

            //     // dup back to stdout
            //     dup2(STDOUT_FILENO, i);
            // }
          }

            
        } // END handle data from client
      } // END got new incoming connection
    } // END looping through file descriptors
  } // END for( ; ; )--and you thought it would never end!

  return 0;
}