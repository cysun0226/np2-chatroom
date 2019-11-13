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

typedef struct {
    int id;
    std::string name = "(no name)";
    std::string ip;
    std::string port;
} User;


extern std::vector<User> user_table;

#endif