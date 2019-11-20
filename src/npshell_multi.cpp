#include "../include/npshell_multi.h" 
#include "../include/parse_multi.h"
#include "../include/execute_multi.h" 
#include "../include/np_multi_proc.h"

bool interrupt = false;

int get_cmd(ConnectInfo info){
  int status = SUCCESS;
  bool use_src = false;

  // prompt
  if (interrupt != true){
    std::cout << PROMPT_SYMBOL << " " << std::flush;
  }
  interrupt = false;
  
  // get user input
  std::vector<Command> cmds;
  std::string usr_input;
  std::getline(std::cin, usr_input);

  // check if interrupt
  if (std::cin.eof()){
    interrupt = true;
    std::cin.clear();
    return SUCCESS;
  }
  
  // remove all \r \n in the input
  usr_input.erase(std::remove(usr_input.begin(), usr_input.end(), '\r'), usr_input.end());
  usr_input.erase(std::remove(usr_input.begin(), usr_input.end(), '\n'), usr_input.end());

  info.usr_input = usr_input;

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
  exec_cmds(parsed_cmd, info);
  status = SUCCESS;
  

  return status;
}


int npshell(ConnectInfo info){

  // set PATH
  char default_path[] = "PATH=bin:.";
  putenv(default_path);

  // set signal of user pipe
  struct sigaction action;
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = &receive_user_pipe;
  sigaction(SIGUSR2, &action, NULL);

  int status;
  do{
    status = get_cmd(info);
  } while (status == SUCCESS);
  
  return 0;
}

