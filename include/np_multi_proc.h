#ifndef NP_MULTI_PROC_H
#define NP_MULTI_PROC_H

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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>


#define BROADCAST_SIG 9999

typedef struct {
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
    char* broadcast_buf;
    char* tell_buf;
    std::string usr_input;
} ConnectInfo;

void broadcast(std::string);
User get_user(int id, User* user_table);
int create_named_pipe(int from, int to);
// extern std::vector<User>* user_table;
#define MAX_USER_NUM 30
#define MAX_TELL_LENGTH 1024
// extern User user_table[MAX_USER_NUM];


#endif