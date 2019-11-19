#ifndef PARSE_MULTI_H
#define PARSE_MULTI_H

#include "npshell_multi.h"
#include "np_multi_proc.h"

std::pair <std::vector<Command>, std::string> parse_cmd(std::string usr_input, ConnectInfo info);

#endif