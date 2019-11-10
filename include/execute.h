#ifndef EXECUTE_H
#define EXECUTE_H

#include "npshell.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <streambuf>

// variables
extern std::vector<Pipe> pipe_table;
extern std::vector< std::pair <int*, int> > table_delete;
extern std::vector<std::pair <int*, int>> tmp_delete;
extern std::map<int, int> out_fd_map;

// functions

void child_handler(int signo);

int build_pipe(std::vector<Command> &cmds);

pid_t exec_cmd(Command cmd, bool last);

int exec_cmds(std::pair<std::vector<Command>, std::string> parsed_cmd);

void clean_up();

void set_env(std::string usr_input);

std::string print_env(std::string);

void update_up_target();

std::string get_cmd_from_source(std::string f_name);

void restore_src_table();

#endif