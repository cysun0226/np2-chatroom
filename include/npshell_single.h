#ifndef NPSHELL_SINGLE_H
#define NPSHELL_SINGLE_H

#include <iostream>
#include <string>
#include <vector>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <utility>
#include <map>

#include "../include/np_single_proc.h"



#define PROMPT_SYMBOL "%" 
#define PIPE_BUFFER_SIZE 15000
enum Status{SUCCESS, EXIT, ERROR, EXECUTE};
enum {READ, WRITE};
enum {PIPE_STDOUT=-1};

typedef struct {
    std::string cmd;
    std::string in_file = "";
    std::string out_file = "";
    int pipe_in = -1;
    int pipe_out = PIPE_STDOUT;
    int in_fd = STDIN_FILENO;
    int out_fd = STDOUT_FILENO;
    int idx = -1;
    std::vector<std::string> args;
    char fd_type;
} Command;

typedef struct {
    int* fd;
    int out_target;
} Pipe;

int npshell(ConnectInfo info);
int run_cmd(std::string usr_input, ConnectInfo info);

#endif