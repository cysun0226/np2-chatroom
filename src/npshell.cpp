#include "../include/npshell.h" 
#include "../include/parse.h"
#include "../include/execute.h" 
#include "../include/server.h"



int get_cmd(ConnectInfo info){
  int status = SUCCESS;
  bool use_src = false;

  // prompt
  std::cout << PROMPT_SYMBOL << " " << std::flush;
  // get user input
  std::vector<Command> cmds;
  std::string usr_input;
  std::getline(std::cin, usr_input);

  // check if built commands
  status = build_in_cmd(usr_input, info);
  if (status != EXECUTE){
    return status;
  }

  // parse
  std::pair<std::vector<Command>, std::string> parsed_cmd\
  = parse_cmd(usr_input, info);

  // check if user exist
  if (cmd_user_exist(parsed_cmd.first, parsed_cmd.second, info) == false){
    return SUCCESS;
  }

  // if no input
  if (parsed_cmd.first.size() < 1){
    return SUCCESS;
  }

  // exec
  exec_cmds(parsed_cmd);
  status = SUCCESS;
  

  return status;
}


int npshell(ConnectInfo info){

  // set PATH
  char default_path[] = "PATH=bin:.";
  putenv(default_path);

  int status;
  do{
    status = get_cmd(info);
  } while (status == SUCCESS);
  
  return 0;
}

