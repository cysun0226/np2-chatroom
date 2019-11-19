#include "../include/npshell.h" 
#include "../include/parse.h"
#include "../include/execute.h" 



int get_cmd(){
  int status = SUCCESS;
  bool use_src = false;

  // prompt
  std::cout << PROMPT_SYMBOL << " " << std::flush;
  // get user input
  std::vector<Command> cmds;
  std::string usr_input;
  std::getline(std::cin, usr_input);

  // if EOF or exit
  if (std::cin.eof() || usr_input.substr(0, 4) == "exit"){
    clean_up();
    return EXIT;
  }

  // env command
  if (usr_input.substr(0, 6) == "setenv"){
    set_env(usr_input.substr(7));
    update_up_target();
    return SUCCESS;
  }
  if (usr_input.substr(0, 8) == "printenv"){
    std::string env_var = print_env(usr_input.substr(9));
    if (env_var != ""){
      std::cout << env_var << std::endl; 
    }
    update_up_target();
    return SUCCESS;
  }

  // source 
  if (usr_input.substr(0, 6) == "source"){
    use_src = true;
    usr_input = get_cmd_from_source(usr_input.substr(7));
  }

  if (use_src){
    std::stringstream ss(usr_input);
    std::string line;

    while (std::getline(ss, line, '\n')){
     // parse
      std::pair<std::vector<Command>, std::string> parsed_cmd\
      = parse_cmd(line); 
      // exec
      exec_cmds(parsed_cmd);
    }
    
  }
  

  // parse
  std::pair<std::vector<Command>, std::string> parsed_cmd\
  = parse_cmd(usr_input);

  // if no input
  if (parsed_cmd.first.size() < 1){
    return SUCCESS;
  }

  // if source, restore
  if (use_src){
  restore_src_table();
   update_up_target();
   return status;
  }

  // exec
  exec_cmds(parsed_cmd);

  

  return status;
}



int npshell(){
  // set PATH
  char default_path[] = "PATH=bin:.";
  putenv(default_path);

  int status;
  do{
    status = get_cmd();
  } while (status == SUCCESS);
  
  return 0;
}