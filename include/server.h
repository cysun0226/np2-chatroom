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
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>


// functions
void fork_handler(int s);
void register_client_child_handle();
int launch_server(std::string port);


#endif