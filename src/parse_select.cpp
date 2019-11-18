#include "../include/parse_select.h"

std::pair <std::vector<Command>, std::string> parse_cmd(std::string usr_input, ConnectInfo info) {
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
                // receive from user pipe
                if (buf[i][0] == '<'){
                    int digit = 2;
                    std::string t_str = std::string(digit - std::to_string(info.id).length(), '0') + std::to_string(info.id);
                    std::string f_str = std::string(digit - str.substr(1).length(), '0') + str.substr(1);
                    cmd.in_file = "./user_pipe/" + f_str + t_str;
                }
                
                else{
                    cmd.args.push_back(buf[i]);
                }
            }
            // output to file
            if (str[0] == '>' && str.size()==1){
                ss >> out_file;
            }   
            // output to user pipe
            if (str[0] == '>' && str.size()>1){
                int digit = 2;
                std::string f_str = std::string(digit - std::to_string(info.id).length(), '0') + std::to_string(info.id);
                std::string t_str = std::string(digit - str.substr(1).length(), '0') + str.substr(1);
                out_file = "./user_pipe/" + f_str + t_str;
                cmd.fd_type = '}';
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
            if (buf[i][0] == '<'){
                    int digit = 2;
                    std::string t_str = std::string(digit - std::to_string(info.id).length(), '0') + std::to_string(info.id);
                    std::string f_str = std::string(digit - str.substr(1).length(), '0') + str.substr(1);
                    cmd.in_file = "./user_pipe/" + f_str + t_str;
                }
                else{
                    cmd.args.push_back(buf[i]);
                }
        }
        cmds.push_back(cmd);
    }
    
    return std::pair<std::vector<Command>, std::string>(cmds, out_file);
}