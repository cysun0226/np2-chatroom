#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <iostream>

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// void broadcast(std::string msg, int fdmax){
//     for(j = 0; j <= fdmax; j++) {
//         if (FD_ISSET(j, &server_fd)) {
//         if (j != listener && j != i) {
//                 if (send(j, buf, nbytes, 0) == -1) {
//                 perror("send");
//             }
//         }
//         }         
//     }
// }

int main(int argc, char* argv[])
{
  // server
  fd_set server_fd;
  fd_set read_fds; // 給 select() 用的暫時 file descriptor 清單
  int fdmax; // 最大的 file descriptor 數目

  int listener; // listening socket descriptor
  int newfd; // 新接受的 accept() socket descriptor
  struct sockaddr_storage client_addr; // client address
  socklen_t addrlen;

  char buf[256]; // 儲存 client 資料的緩衝區
  int nbytes;

  char remoteIP[INET6_ADDRSTRLEN];

  int yes=1; // 供底下的 setsockopt() 設定 SO_REUSEADDR
  int i, j, rv;

  struct addrinfo hints, *ai, *p;

  FD_ZERO(&server_fd); // 清除 master 與 temp sets
  FD_ZERO(&read_fds);

  // 給我們一個 socket，並且 bind 它
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

    // 避開這個錯誤訊息："address already in use"
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
      close(listener);
      continue;
    }

    break;
  }

  // 若我們進入這個判斷式，則表示我們 bind() 失敗
  if (p == NULL) {
    fprintf(stderr, "selectserver: failed to bind\n");
    exit(2);
  }
  freeaddrinfo(ai); // all done with this

  // listen
  if (listen(listener, 10) == -1) {
    perror("listen");
    exit(3);
  }

  // 將 listener 新增到 master set
  FD_SET(listener, &server_fd);

  // 持續追蹤最大的 file descriptor
  fdmax = listener; // 到此為止，就是它了
  std::cout << "wait for connection ... " << std::endl;

  // 主要迴圈
  for( ; ; ) {
    read_fds = server_fd; // 複製 master

    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    // 在現存的連線中尋找需要讀取的資料
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // 我們找到一個！！
        if (i == listener) {
          // handle new connections
          socklen_t addr_size = sizeof client_addr;
          newfd = accept(listener, (struct sockaddr *)&client_addr, &addr_size);

          if (newfd == -1) {
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
            }

            // add new client to server_fd
            FD_SET(newfd, &server_fd);

            // keep track of the max fd
            if (newfd > fdmax) {
              fdmax = newfd;
            }
          }

        } 

        // client main
        else {
            // connection close
            if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            // got error or connection closed by client
            if (nbytes == 0) {
              printf("selectserver: socket %d hung up\n", i);
            } else {
              perror("recv");
            }
            // close connection
            close(i);
            // remove from server fd  list
            FD_CLR(i, &server_fd);

          }
          // get input from user
          else{
            pid_t pid = fork();
            if (pid == 0){
                dup2(i, STDOUT_FILENO);
                close(i);
                execlp("bin/ls", "bin/ls", (char*) NULL);
            }
          }

            
        } // END handle data from client
      } // END got new incoming connection
    } // END looping through file descriptors
  } // END for( ; ; )--and you thought it would never end!

  return 0;
}