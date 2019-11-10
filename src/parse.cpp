#include "parse.h"

std::pair <std::vector<Command>, std::string> parse_cmd(std::string usr_input) {
    std::vector<Command> cmds;
    std::string out_file = "";
    std::stringstream ss;
    ss.str(usr_input);
    // ss.exceptions(std::ios::failbit);
    std::vector<std::string> buf;
    std::string str;
    int idx = 0;
    while(ss >> str){
        if (str[0] == '|' || str[0] == '!' || str[0] == '>'){
            Command cmd;
            cmd.idx = idx; idx++;
            cmd.fd_type = str[0];
            // the pipe out idx
            if (str.size()>1){
                cmd.pipe_out = std::stoi(str.substr(1));
            }
            cmd.cmd = buf[0];
            for (size_t i = 0; i < buf.size(); i++){
                cmd.args.push_back(buf[i]);
            }
            if (str[0] == '>'){
                ss >> out_file;
            }   
            cmds.push_back(cmd);
            buf.clear();
        }
        else{
            buf.push_back(str);
        }
    }

    // last cmd (to stdout)
    if (buf.size() > 0){
        Command cmd;
        cmd.idx = idx; idx++;
        cmd.fd_type = '-';
        cmd.cmd = buf[0];
        for (size_t i = 0; i < buf.size(); i++){
            cmd.args.push_back(buf[i]);
        }
        cmds.push_back(cmd);
    }
    
    return std::pair<std::vector<Command>, std::string>(cmds, out_file);
}