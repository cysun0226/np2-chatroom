#ifndef PARSE_SELECT_H
#define PARSE_SELECT_H

#include "npshell_select.h"
#include "select_server.h"

std::pair <std::vector<Command>, std::string> parse_cmd(std::string usr_input, ConnectInfo info);

#endif