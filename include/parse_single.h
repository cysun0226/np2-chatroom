#ifndef PARSE_SINGLE_H
#define PARSE_SINGLE_H

#include "npshell_single.h"
#include "np_single_proc.h"

std::pair <std::vector<Command>, std::string> parse_cmd(std::string usr_input, ConnectInfo info);

#endif