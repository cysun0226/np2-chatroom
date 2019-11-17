#include "../include/parse.h"
#include "../include/npshell.h"

int main() 
{
    // get user input
    std::vector<Command> cmds;
    std::string usr_input;
    std::getline(std::cin, usr_input);

    // parse
    std::pair<std::vector<Command>, std::string> parsed_cmd\
    = parse_cmd(usr_input);

    cmds = parsed_cmd.first;

    for (size_t i = 0; i < cmds.size(); i++){
        std::cout << "cmd " << i << std::endl;
        std::cout << cmds[i].cmd << std::endl;
        std::cout << "fd_type = " << cmds[i].fd_type << std::endl;
    }
    std::cout << "out_file = " << parsed_cmd.second << std::endl;

    return 0;
}