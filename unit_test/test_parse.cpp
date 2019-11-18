#include "../include/parse.h"
#include "../include/npshell.h"
#include "../include/server.h"

int main() 
{
    // get user input
    std::vector<Command> cmds;
    std::string usr_input;
    std::getline(std::cin, usr_input);

    // create a connect info
    ConnectInfo ci;
    ci.id = 4;

    // parse
    std::pair<std::vector<Command>, std::string> parsed_cmd\
    = parse_cmd(usr_input, ci);

    cmds = parsed_cmd.first;

    for (size_t i = 0; i < cmds.size(); i++){
        std::cout << "\ncmd " << i << std::endl;
        std::cout << cmds[i].cmd << std::endl;
        std::cout << "fd_type = " << cmds[i].fd_type << std::endl;
        std::cout << "in_file = " << cmds[i].in_file << std::endl;
    }
    std::cout << "\nout_file = " << parsed_cmd.second << std::endl;

    return 0;
}