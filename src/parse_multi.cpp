#include "../include/parse_multi.h"

std::pair <std::vector<Command>, std::string> parse_cmd(std::string usr_input, ConnectInfo info) {
    std::vector<Command> cmds;
    std::string out_file = "";
    std::stringstream ss;
    ss.str(usr_input);
    // ss.exceptions(std::ios::failbit);
    std::vector<std::string> buf;
    std::string str;
    int idx = 0;
    std::vector <std::string> tokens;

    while(ss >> str){
        tokens.push_back(str);
    }
    
    bool skip = false;
    for (size_t t = 0; t < tokens.size(); t++){
        if (skip){
            skip = false;
            continue;
        }
        str = tokens[t];
        if (str[0] == '|' || str[0] == '!' || str[0] == '>' || str[0] == '<'){
            Command cmd;
            cmd.idx = idx; idx++;
            cmd.fd_type = str[0];
            // the pipe out idx
            if (str[0] == '|' && str.size()>1){
                cmd.pipe_out = std::stoi(str.substr(1));
            }
            cmd.cmd = buf[0];

            // output file name
            if ((str[0] == '>' && str.size()>1) || str[0] == '<'){
                buf.push_back(str);
            }

            // test if next token is also pipe
            if (t != tokens.size()-1 && 
                ((tokens[t+1][0] == '>' && tokens[t+1].size()>1) || tokens[t+1][0] == '<' || 
                  tokens[t+1][0] == '|')){
                    buf.push_back(tokens[t+1]);
                    skip = true;
            }

            // if output to file
            if (str[0] == '>' && str.size()==1){
                out_file = tokens[t+1];
            }

            for (size_t i = 0; i < buf.size(); i++){
                // receive from user pipe
                if (buf[i][0] == '<'){
                    int digit = 2;
                    std::string t_str = std::string(digit - std::to_string(info.id).length(), '0') + std::to_string(info.id);
                    std::string f_str = std::string(digit - buf[i].substr(1).length(), '0') + buf[i].substr(1);
                    cmd.in_file = "./user_pipe/" + f_str + t_str;
                }
                // output to user pipe
                else if (buf[i][0] == '>' && buf[i].size()>1){
                    int digit = 2;
                    std::string f_str = std::string(digit - std::to_string(info.id).length(), '0') + std::to_string(info.id);
                    std::string t_str = std::string(digit - buf[i].substr(1).length(), '0') + buf[i].substr(1);
                    cmd.out_file = "./user_pipe/" + f_str + t_str;
                    cmd.fd_type = '}';
                }
                else if (buf[i][0] == '|'){
                    if (buf[i].size()>1) {
                        cmd.pipe_out = std::stoi(buf[i].substr(1));
                    }
                    cmd.fd_type = '|';
                }
                else{
                    cmd.args.push_back(buf[i]);
                }
            }
            cmds.push_back(cmd);
            buf.clear();
        }
        else{
            buf.push_back(str);
        }
    }

    if (out_file != ""){
        return std::pair<std::vector<Command>, std::string>(cmds, out_file);    
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

// int main(){
//     // create a connect info
//     ConnectInfo ci;
//     ci.id = 99;
//     std::string usr_input;
//     std::getline(std::cin, usr_input);
//     std::pair<std::vector<Command>, std::string> parsed_cmd\
//     = parse_cmd(usr_input, ci);
// }