#ifndef SERVER_H
#define SERVER_H

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

#define BROADCAST_SIG -1

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
} ConnectInfo;

void broadcast(std::string);
User get_user(int id, User* user_table);
// extern std::vector<User>* user_table;
#define MAX_USER_NUM 30
#define MAX_TELL_LENGTH 1024
// extern User user_table[MAX_USER_NUM];


#endif