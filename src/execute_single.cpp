#include "../include/execute_single.h"
#include "../include/np_single_proc.h"

// std::vector<Pipe> pipe_table;
std::vector< std::pair <int*, int> > table_delete;
std::vector<std::pair <int*, int>> tmp_delete;
std::map<int, int> out_fd_map;

std::vector<Pipe> pipe_table_bk;
std::vector< std::pair <int*, int> > table_delete_bk;
std::vector<std::pair <int*, int>> tmp_delete_bk;


std::vector<SendPipe> send_table;
std::vector<UserPipe> user_pipe_table;


int get_user_pipe_table_idx(int id){
  for (size_t i = 0; i < user_pipe_table.size(); i++){
    if (id == user_pipe_table[i].id){
      return i;
    }
  }
  return -1;
}

void child_handler(int signo){
    int status;
    // -1 : wait for any child
    while (waitpid(-1, &status, WNOHANG) > 0);
}

pid_t exec_cmd(Command cmd, bool last){
  int status;
  pid_t pid;
  bool fork_failed;
  int pipe_id = 0;

  // check if exit
  if (cmd.cmd == "exit"){
    return EXIT;
  }

  // fork child process
  pid = fork();

  // if can't fork, sleep and wait for nect fork
  fork_failed = (pid < 0)? true:false;
  if (fork_failed){
    while (pid < 0) {
      usleep (1000);
      pid = fork();
    }
  }

  switch (pid){
  // child
  case 0:{
    // reditect I/O
    dup2(cmd.in_fd, STDIN_FILENO);  
    dup2(cmd.out_fd, STDOUT_FILENO);
    if (cmd.fd_type == '!'){
      dup2(cmd.out_fd, STDERR_FILENO);
    }

    // if receive from user pipe
    if (cmd.in_file != ""){
      close(cmd.in_fd);
    }
    if (cmd.fd_type == '}'){
      close(cmd.out_fd);
    }
    

    // close unuse pipes
    for (size_t i = 0; i < tmp_delete.size(); i++){
      close(tmp_delete[i].first[READ]);
      close(tmp_delete[i].first[WRITE]);
    }
    for (size_t i = 0; i < table_delete.size(); i++){
      close(table_delete[i].first[READ]);
      close(table_delete[i].first[WRITE]);
    }
    
    // convert cmd.args to exec format
    // const char *p_name = cmd.cmd.c_str();
    const char **args = new const char* [cmd.args.size()+1];
    // args[0] = p_name;
    for (int i = 0; i < cmd.args.size(); i++){
        args[i] = cmd.args[i].c_str();
    }   
    args[cmd.args.size()] = NULL;

    // execute    
    status = execvp(args[0], (char**)args); // process name, args
    std::cerr << "Unknown command: [" << args[0] << "]." << std::endl;

    exit(status);
    break;
  }
  
  // pid > 0, parent
  default:{
    // close useless pipe
    // for (size_t i = 0; i < tmp_delete.size(); i++){
    //   if (cmd.in_fd == tmp_delete[i].first[READ]){
    //     close(cmd.in_fd); 
    //   }
    //   if(cmd.idx == tmp_delete[i].second){
    //     close(tmp_delete[i].first[WRITE]);
    //   }
    // }

    if (last){
      // close pipe
      for (size_t i = 0; i < tmp_delete.size(); i++){      
        close(tmp_delete[i].first[READ]);
        close(tmp_delete[i].first[WRITE]);
      }
      for (size_t i = 0; i < table_delete.size(); i++){
        close(table_delete[i].first[READ]);
        close(table_delete[i].first[WRITE]);
      }
    }

    // use signal handler to catch child that is not output to stdout
    if (cmd.out_fd != STDOUT_FILENO){
      if (last && cmd.fd_type=='>') {
        close(cmd.out_fd);
        waitpid(pid, &status, 0);
      }
      else{
        if (last && cmd.fd_type=='}') {
          close(cmd.out_fd);
        }
        signal(SIGCHLD, child_handler);
      }
    }
    else{
      // wait for the stdout process
      waitpid(pid, &status, 0);
    }
    
    break;
  }
  }

  return pid;
}

int build_pipe(std::vector<Command> &cmds, std::string filename, ConnectInfo info){
  std::vector<int*> fd_list;
  int table_idx = get_user_pipe_table_idx(info.id);
  /* Check if previous pipe occurs */
  for (size_t i = 0; i < cmds.size(); i++){
    for (size_t p = 0; p < user_pipe_table[table_idx].pipe_table.size(); p++){
      // exist pipe to stdin
      if (user_pipe_table[table_idx].pipe_table[p].out_target ==  cmds[i].idx){
          cmds[i].in_fd = user_pipe_table[table_idx].pipe_table[p].fd[READ];
          std::pair <int*, int> table_entry(user_pipe_table[table_idx].pipe_table[p].fd, p);
          // can delete after used
          table_delete.push_back(table_entry);
      }
      // output target has existing pipe
      if (cmds[i].pipe_out == user_pipe_table[table_idx].pipe_table[p].out_target){
          cmds[i].out_fd = user_pipe_table[table_idx].pipe_table[p].fd[WRITE];
          out_fd_map[cmds[i].out_fd] = i;  //  update last user of this pipe
      }
      // next input has existing pipe
      if (cmds[i].pipe_out == PIPE_STDOUT && cmds[i].idx+1==user_pipe_table[table_idx].pipe_table[p].out_target &&
          cmds[i].fd_type != '-'){
          cmds[i].out_fd = user_pipe_table[table_idx].pipe_table[p].fd[WRITE];
          out_fd_map[cmds[i].out_fd] = i;  //  update last user of this pipe
      }
    }
  }

  /* if receive input from user pipe */
  for (size_t i = 0; i < cmds.size(); i++){
    if (cmds[i].in_file != ""){
      int from = std::stoi(cmds[i].in_file.substr(12, 2));
      int to = std::stoi(cmds[i].in_file.substr(14, 2));
      for (size_t i = 0; i < send_table.size(); i++){
        if (send_table[i].from == from && send_table[i].to == to){
          cmds[i].in_fd = send_table[i].fd[READ];
          break;
        }
      }

      // broadcast receive
      std::stringstream ss;
      ss << "*** " << get_user_by_id(to).name << " (#" << to << ") just received from " \
      << get_user_by_id(from).name << " (#" << from << ") by '" << info.usr_input << "' ***\n";
      broadcast(ss.str());
    }
  }
  

  /* if output to file */
  int outfile_fd;
  if (cmds.back().fd_type == '>'){
    outfile_fd = open(filename.c_str(),
    O_WRONLY | O_CREAT | O_TRUNC,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    cmds.back().out_fd = outfile_fd;
  }

  /* if write to user pipe */
  if (cmds.back().fd_type == '}'){
    int from = std::stoi(filename.substr(12, 2));
    int to = std::stoi(filename.substr(14, 2));

    // open a pipe
    SendPipe up;
    up.from = from;
    up.to = to;
    
    if (pipe(up.fd) < 0){
      std::cerr << "[open user pipe error]" << std::endl;
    }

    send_table.push_back(up);    
    cmds.back().out_fd = up.fd[WRITE];

    // broadcast user pipe create
    std::stringstream ss;
    ss << "*** " << get_user_by_id(from).name << " (#" << from << ") just piped '"\
    << info.usr_input << "' to " << get_user_by_id(to).name << " (#" << to << ") ***\n";
    broadcast(ss.str());
  }


  /* Create required new pipes */
  for (size_t i = 0; i < cmds.size(); i++){
      // no previous existing pipe for next command
      if (cmds[i].pipe_out == PIPE_STDOUT &&
          cmds[i].out_fd == STDOUT_FILENO &&
          cmds[i+1].in_fd == STDIN_FILENO &&
          i != cmds.size()-1)  // the last command use stdout
      {
          // allocate new pipe
          int* fd = new int[2];
          if (pipe(fd) < 0){
              std::cerr << "[pipe error]" << cmds[i].cmd << std::endl;
              break;
          }
          
          cmds[i].out_fd = fd[WRITE];
          cmds[i+1].in_fd = fd[READ];
          
          out_fd_map[cmds[i].out_fd] = i;  //  update last user of this pipe
          fd_list.push_back(fd);
          std::pair <int*, int> table_entry(fd, i+1);
          tmp_delete.push_back(table_entry);
      }      

      // no existing pipe for the output target
      if (cmds[i].pipe_out != PIPE_STDOUT && cmds[i].out_fd == STDOUT_FILENO){
          // allocate new pipe
          int* fd = new int[2];
          if (pipe(fd) < 0){
              std::cerr << "[pipe error]" << cmds[i].cmd << std::endl;
              break;
          }
          fd_list.push_back(fd);

          cmds[i].out_fd = fd[WRITE];
          // output target is in the current cmds
          if (cmds[i].pipe_out < cmds.size()-cmds[i].idx){
              cmds[i+cmds[i].pipe_out].in_fd = fd[READ];
              std::pair <int*, int> table_entry(fd, i+1);
              // can delete after used
              tmp_delete.push_back(table_entry);
              // redirct all cmd.out_fd that output to the same cmd
              for (size_t j = i+1; j<cmds.size(); j++){
                if (cmds[j].pipe_out != PIPE_STDOUT){
                  if (cmds[j].pipe_out + cmds[j].idx == cmds[i].pipe_out+cmds[i].idx){
                    cmds[j].out_fd = cmds[i].out_fd;
                    out_fd_map[cmds[i].out_fd] = i;  //  update last user of this pipe
                  }
                }
                else{
                  if (cmds[i].idx+cmds[i].pipe_out==cmds[j].idx+1){
                    cmds[j].out_fd = cmds[i].out_fd;
                    out_fd_map[cmds[i].out_fd] = i;  //  update last user of this pipe
                  } 
                }
              }
          }
          // relay to following commands
          else{
              // redirct all cmd.out_fd that output to the same cmd
              for (size_t j = i+1; j<cmds.size(); j++){
                if (cmds[j].pipe_out != PIPE_STDOUT){
                  if (cmds[j].pipe_out + cmds[j].idx == cmds[i].pipe_out+cmds[i].idx){
                    cmds[j].out_fd = cmds[i].out_fd;
                  }
                }
                else{
                  if (cmds[i].idx+cmds[i].pipe_out==cmds[j].idx+1){
                    cmds[j].out_fd = cmds[i].out_fd;      
                  } 
                }
              }
              // update pipe_out for following commands
              cmds[i].pipe_out += (cmds[i].idx);
              Pipe p = {fd, cmds[i].pipe_out};
              user_pipe_table[table_idx].pipe_table.push_back(p);
          }
      }

      // execute
      exec_cmd(cmds[i], i==cmds.size()-1);
  }

  

  return outfile_fd;
}

int exec_cmds(std::pair<std::vector<Command>, std::string> parsed_cmd, ConnectInfo info){
    std::vector<Command> cmds = parsed_cmd.first;
    int status;
    pid_t last_pid;

    // build pipes
    int outfile_fd = build_pipe(cmds, parsed_cmd.second, info);

    // remove used user pipes
    for (size_t i = 0; i < cmds.size(); i++){
      if (cmds[i].in_file != ""){
        int from = std::stoi(cmds[i].in_file.substr(12, 2));
        int to = std::stoi(cmds[i].in_file.substr(14, 2));
        for (size_t i = 0; i < send_table.size(); i++){
          if (send_table[i].from == from && send_table[i].to == to){
            close(send_table[i].fd[READ]);
            close(send_table[i].fd[WRITE]);
            send_table[i] = send_table.back();
            send_table.pop_back();
          }
        }
      }
    }

    // execute commands
    // for (size_t i = 0; i < cmds.size(); i++){
    //     exec_cmd(cmds[i], i==cmds.size()-1);
    // }
    
    // delete tmp pipes for current cmds
    for (size_t i = 0; i < tmp_delete.size(); i++){
        delete [] tmp_delete[i].first;
    }
    tmp_delete.clear();
    out_fd_map.clear();

    // delete used pipe in pipe_table
    int table_idx = get_user_pipe_table_idx(info.id);
    for (size_t i = 0; i < table_delete.size(); i++){
        // delete fd
        delete[] table_delete[i].first; 
        // delete entry in pipe_table
        user_pipe_table[table_idx].pipe_table[table_delete[i].second] = user_pipe_table[table_idx].pipe_table.back();
        user_pipe_table[table_idx].pipe_table.pop_back();
    }
    table_delete.clear();

    // update out_target in pipe_table
    for (size_t i = 0; i < user_pipe_table[table_idx].pipe_table.size(); i++){
        user_pipe_table[table_idx].pipe_table[i].out_target -= cmds.size();
    }

    return status;
}

void clean_up(ConnectInfo info){
    // delete tmp pipes for current cmds
    for (size_t i = 0; i < tmp_delete.size(); i++){
        delete [] tmp_delete[i].first;
    }
    tmp_delete.clear();

    // delete used pipe in pipe_table
    int table_idx = get_user_pipe_table_idx(info.id);
    for (size_t i = 0; i < table_delete.size(); i++){
        // delete fd
        delete[] table_delete[i].first; 
        // delete entry in pipe_table
        user_pipe_table[table_idx].pipe_table[table_delete[i].second] = user_pipe_table[table_idx].pipe_table.back();
        user_pipe_table[table_idx].pipe_table.pop_back();
    }
    table_delete.clear();

    for (size_t i = 0; i < user_pipe_table[table_idx].pipe_table.size(); i++){
        delete[] user_pipe_table[table_idx].pipe_table[i].fd;
    }
}

void set_env(std::string usr_input, int id) {
  std::stringstream ss;
  ss.str(usr_input);
  std::string var, value, cmd_str;
  ss >> var >> value;

  setenv(var.c_str(), value.c_str(), 1);
  update_user_env(id, var, value);
}

std::string print_env(std::string usr_input){
  std::stringstream ss;
  ss.str(usr_input);
  std::string var_str, cmd_str;
  ss >> var_str;
  char* var = new char[var_str.size()+1];
  strcpy(var, var_str.c_str());

  char* ptr = getenv(var);
  delete [] var;
  if (ptr == nullptr){
    return "";
  }

  return std::string(ptr);
}

void update_target(ConnectInfo info){
  int table_idx = get_user_pipe_table_idx(info.id);
  // update out_target in pipe_table
    for (size_t i = 0; i < user_pipe_table[table_idx].pipe_table.size(); i++){
        user_pipe_table[table_idx].pipe_table[i].out_target -= 1;
    }
}

// std::string get_cmd_from_source(std::string f_name){
//   std::stringstream ss;
//   ss.str(f_name);
//   std::string file_name;
//   ss >> file_name;

//   std::ifstream t(file_name);
//   std::string cmd_file(
//     (std::istreambuf_iterator<char>(t)),
//     std::istreambuf_iterator<char>()
//   );

//   // back up pipes
//   // std::vector<Pipe> pipe_table_bk;
//   pipe_table_bk.clear();
//   pipe_table_bk.assign(pipe_table.begin(), pipe_table.end());
//   pipe_table.clear();

//   return cmd_file;

// }

// void restore_src_table(){
//   pipe_table.clear();
//   pipe_table.assign(pipe_table_bk.begin(), pipe_table_bk.end()); 
// }

bool cmd_user_exist(std::vector<Command> cmds, std::string out_file, ConnectInfo info){
  for (size_t i = 0; i < cmds.size(); i++) {
    // sender
    if (cmds[i].fd_type == '}'){
      int from = std::stoi(out_file.substr(12, 2));
      int to = std::stoi(out_file.substr(14, 2));

      // user not exist
      User to_user = get_user_by_id(to);
      if (to_user.id == -1){
        std::cout << "*** Error: user #" << to << " does not exist yet. ***" << std::endl;
        return false;
      }
      
      // if named pipe exist
      for (size_t i = 0; i < send_table.size(); i++){
        if (send_table[i].to == to && send_table[i].from == from){
          std::cout << "*** Error: the pipe #" << from << "->#" \
          << to << " already exists. ***" << std::endl;
          return false;
        }
      }
    }

    // receiver
    if (cmds[i].in_file != ""){
      int from = std::stoi(cmds[i].in_file.substr(12, 2));
      int to = std::stoi(cmds[i].in_file.substr(14, 2));
      
      // if user exist
      User from_user = get_user_by_id(from);
      if (from_user.id == -1){
        std::cout << "*** Error: user #" << from << " does not exist yet. ***" << std::endl;
        return false;
      }

      // if named pipe exist
      bool exist = false;
      for (size_t i = 0; i < send_table.size(); i++){
        if (send_table[i].to == info.id && send_table[i].from == from_user.id){
          exist = true;
        }
      }
      if (exist == false){
         std::cout << "*** Error: the pipe #" << from << "->#" \
          << to << " does not exist yet. ***" << std::endl;
           return false;
      }
    }
  }
  
  return true;
}


// build-in cmd ---------------------------------------------------------------------------------------

int build_in_cmd(std::string usr_input, ConnectInfo info){ 
  // if EOF or exit
  if (std::cin.eof() || usr_input.substr(0, 4) == "exit"){
    clean_up(info);
    return EXIT;
  }

  // env command
  if (usr_input.substr(0, 6) == "setenv"){
    set_env(usr_input.substr(7), info.id);
    update_target(info);
    return SUCCESS;
  }
  if (usr_input.substr(0, 8) == "printenv"){
    std::string env_var = print_env(usr_input.substr(9));
    if (env_var != ""){
      std::cout << env_var << std::endl; 
    }
    update_target(info);
    return SUCCESS;
  }

  // who
  if (usr_input.substr(0, 3) == "who"){
    who(info.id, get_user_by_id(info.id).fd);
    return SUCCESS;
  }

  // name
  if (usr_input.substr(0, 4) == "name"){
    std::stringstream ss;
    ss.str(usr_input.substr(5));
    std::string user_name;
    ss >> user_name;
    name(info.id, user_name);
    return SUCCESS;
  }

  // yell
  if (usr_input.substr(0, 4) == "yell"){
    std::string msg = usr_input.substr(5);
    yell(info.id, msg);
    return SUCCESS;
  }

  // tell
  if (usr_input.substr(0, 4) == "tell"){
    std::string usr_msg = usr_input.substr(5);
    std::stringstream ss;
    ss.str(usr_msg);
    int to_id;
    std::string first_word;
    ss >> to_id >> first_word;

    // check if exist
    User to_user = get_user_by_id(to_id);
    if (to_user.id == -1){
      std::cout << "*** Error: user #" << to_id << " does not exist yet. ***" << std::endl;
      return SUCCESS;
    }

    std::string msg = usr_msg.substr(usr_msg.find(first_word));
    std::string tell_msg = 
    "*** " + std::string(get_user_by_id(info.id).name) + " told you ***: " + msg;
    tell(info.id, to_id, tell_msg);
    return SUCCESS;
  }

  return EXECUTE;
}


void remove_user_pipe(int id){
  for (size_t i = 0; i < send_table.size(); i++){
    if (send_table[i].to == id){
      close(send_table[i].fd[WRITE]);
      close(send_table[i].fd[READ]);
      send_table[i] = send_table.back();
      send_table.pop_back();
    }
  }
}

void pipe_table_add_user(int id){
  UserPipe up;
  up.id = id;
  user_pipe_table.push_back(up);
}

void pipe_table_remove_user(int id){
  for (size_t i = 0; i < user_pipe_table.size(); i++){
    if (user_pipe_table[i].id == id){
      for (size_t j = 0; j < user_pipe_table[i].pipe_table.size(); j++){
        close(user_pipe_table[i].pipe_table[j].fd[READ]);
        close(user_pipe_table[i].pipe_table[j].fd[WRITE]);
        delete [] user_pipe_table[i].pipe_table[j].fd;
      }
      user_pipe_table[i] = user_pipe_table.back();
      user_pipe_table.pop_back();
      return;
    }
  }
}
