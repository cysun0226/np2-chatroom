#ifndef NP_SIMPLE_H
#define NP_SIMPLE_H

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

#define MAX_USER_NUM 1




#endif