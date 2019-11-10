#ifndef PARSE_H
#define PARSE_H

#include "npshell.h"

std::pair <std::vector<Command>, std::string> parse_cmd(std::string usr_input);

#endif