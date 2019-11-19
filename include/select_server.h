#ifndef SELECT_SERVER_H
#define SELECT_SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <netinet/in.h>
#include <sstream>


#define BROADCAST_SIG -1

typedef struct {
    int socket_id;
    int id;
    char name[30];
    char ip[NI_MAXHOST];
    char port[NI_MAXSERV];
    pid_t pid;
    int fd;
    bool clear;
} User;

typedef struct {
    int id;
    User* user_table;
    std::string usr_input;
} ConnectInfo;

typedef struct {
    int from;
    int to;
    int fd[2];
} UserPipe;


// void broadcast(std::string);
User get_user_by_id(int id);
void who(int id, int client_fd);
void name(int id, std::string user_name);
void yell(int id, std::string msg);
void tell(int from, int to, std::string msg);
void broadcast(std::string msg);
// int create_named_pipe(int from, int to);
// extern std::vector<User>* user_table;
#define MAX_USER_NUM 30
#define MAX_TELL_LENGTH 1024
#define MAX_INPUT_LENGTH 15000


// extern User user_table[MAX_USER_NUM];


#endif