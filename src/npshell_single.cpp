#include "../include/npshell_single.h" 
#include "../include/parse_single.h"
#include "../include/execute_single.h" 
#include "../include/np_single_proc.h"

int run_cmd(std::string usr_input, ConnectInfo info){
  // get user input
  std::vector<Command> cmds;

  // check if built commands
  int status = build_in_cmd(usr_input, info);
  if (status != EXECUTE){
    return status;
  }

  // parse
  std::pair<std::vector<Command>, std::string> parsed_cmd\
  = parse_cmd(usr_input, info);

  // check if user exist
  if (cmd_user_exist(parsed_cmd.first, parsed_cmd.second, info) == false){
    update_target(info);
    return SUCCESS;
  }

  // if no input
  if (parsed_cmd.first.size() < 1){
    return SUCCESS;
  }

  // exec
  exec_cmds(parsed_cmd, info);
  status = SUCCESS;
  

  return status;
}




