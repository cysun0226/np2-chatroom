#include "../include/np_single_proc.h"
#include "../include/npshell_single.h"
#include "../include/execute_single.h"
#include "../include/server.h"


User user_table[MAX_USER_NUM];
std::vector<UserEnv> user_env;


// user -------------------------------------------------------------------------
int add_user(char* ip, char* port, int client_fd){
    int new_user_id;
    
    // find the smallest unused id
    std::vector <int> used_id;
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id != -1){
            used_id.push_back(user_table[i].id);
        }
    }
    
    for (size_t i = 1; i < MAX_USER_NUM; i++){
        // if i not in used_id
        if (std::find(used_id.begin(), used_id.end(), i) == used_id.end()){
            new_user_id = i;
            break;
        }
    }

    // find an empty entry
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id == -1){
            user_table[i].id = new_user_id;
            strcpy(user_table[i].ip, ip);
            strcpy(user_table[i].name, "(no name)");
            strcpy(user_table[i].port, port);
            user_table[i].fd = client_fd;
            break;
        }
    }

    pipe_table_add_user(new_user_id);

    return new_user_id;
}

void init_user_table(){
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        user_table[i].id = -1;
        user_table[i].clear = true;
    }
}

void update_user_env(int id, std::string name, std::string value){
  // add to user_env_table
  for (size_t i = 0; i < user_env.size(); i++){
    if (user_env[i].id == id){
      user_env[i].env_var[name] = value;
      break;
    }
  }
}

void set_user_env(int id){
  // set env var
  for (size_t i = 0; i < user_env.size(); i++){
    if (user_env[i].id == id){
      std::map<std::string, std::string>::iterator iter;
      for(iter = user_env[i].env_var.begin(); iter != user_env[i].env_var.end(); iter++){
        setenv(iter->first.c_str(), iter->second.c_str(), 1);
      }
      break;
    }
  }
}

void restore_user_env(int id){
  // set env var
  for (size_t i = 0; i < user_env.size(); i++){
    if (user_env[i].id == id){
      std::map<std::string, std::string>::iterator iter;
      for(iter = user_env[i].env_var.begin(); iter != user_env[i].env_var.end(); iter++){
        unsetenv(iter->first.c_str());
      }
      break;
    }
  }
}

void remove_env_var(int id){
  // remove env var
      for (size_t j = 0; j < user_env.size(); j++){
        if (user_env[j].id == id){;
          // user_env.at(j) = user_env.back();
          // user_env.pop_back();
          user_env.erase(user_env.begin()+j);
          break;
        }
      }
}

void remove_user(int id){
  remove_user_pipe(id);
  remove_env_var(id);
  pipe_table_remove_user(id);
  for (size_t i = 0; i < MAX_USER_NUM; i++){
        // std::cout << "user[i].id = " << user_table[i].id << " id = " << id << std::endl;
        if (user_table[i].id == id){
            user_table[i].id = -1;
            break;
        }
  }
}

User get_user_by_id(int id){
  for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (id == user_table[i].id){
            return user_table[i];
        }
    }
    // not exist
    User u;
    u.id = -1;
    strcpy(u.name, "not_exist");
    return u;
}

User get_user_by_fd(int fd){
  for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (fd == user_table[i].fd){
            return user_table[i];
        }
    }
    // not exist
    User u;
    u.id = -1;
    strcpy(u.name, "not_exist");
    return u;
}

// build-in commands
void broadcast(std::string msg){
  for (size_t i = 0; i < MAX_USER_NUM; i++){
    if (user_table[i].id != -1){
      send(user_table[i].fd, msg.c_str(), msg.length(), 0);
    }
  }
}

void who(int id, int client_fd){
  std::string header = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
  send(client_fd, header.c_str(), header.length(), 0);

  for (size_t i = 0; i < MAX_USER_NUM; i++){
    if (user_table[i].id != -1){
      std::stringstream ss;
      ss << user_table[i].id << "\t" 
              << user_table[i].name << "\t"
              << user_table[i].ip << ":"
              << user_table[i].port;
      if (user_table[i].id == id){
        ss << "\t<-me";
      }
      ss << "\n";
      send(client_fd, ss.str().c_str(), ss.str().length(), 0);
    }
  }
}

void name(int id, std::string user_name){
  // check if name exist
  for (size_t i = 0; i < MAX_USER_NUM; i++){
    if (user_table[i].id != -1){
      if (strcmp(user_name.c_str(), user_table[i].name) == 0){
        std::stringstream ss;
        ss << "*** User '" << user_name << "' already exists. ***\n";
        send(get_user_by_id(id).fd, ss.str().c_str(), ss.str().length(), 0);
        return;
      }
    }
  }

  // update user name
  for (size_t i = 0; i < MAX_USER_NUM; i++){
    if (user_table[i].id == id){
      strcpy(user_table[i].name, user_name.c_str());
      std::string name_msg = 
      "*** User from " + std::string(user_table[i].ip) + ":" + \
      std::string(user_table[i].port) + " is named '" + user_name + "'. ***\n";
      broadcast(name_msg);
      break;
    }
  }
}

void yell(int id, std::string msg){
  std::string yell_msg = 
      "*** " + std::string(get_user_by_id(id).name) + " yelled ***: " + msg + "\n";
  broadcast(yell_msg);
}

void tell(int from, int to, std::string msg){
  std::string tell_msg = msg + "\n";
  send(get_user_by_id(to).fd, tell_msg.c_str(), tell_msg.length(), 0);
}


// main ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // check if input port
    if (argc < 2) {
        std::cout << "please input port!" << std::endl;
        exit(0);
    }
  std::string port = argv[1];
  
  // set PATH
  char default_path[] = "PATH=bin:.";
  putenv(default_path);
  
  // server data
  // fd
  fd_set server_fd;
  fd_set client_fds;
  FD_ZERO(&server_fd);
  FD_ZERO(&client_fds);
  int max_fd, client_fd;
  // user
  init_user_table();
  struct sockaddr_storage client_addr;
  char client_input[MAX_INPUT_LENGTH];
  int user_input_size;

  // launch server
  int listener = launch_server(port);
  FD_SET(listener, &server_fd); // add listener to the server_fd

  // keep track on the max fd
  max_fd = listener;

  std::cout << "wait for connection ... " << std::endl;

  // main loop
  while(true) {

    client_fds = server_fd;

    int select_status = select(max_fd+1, &client_fds, NULL, NULL, NULL);
    if (select_status == -1) {
      continue;
    }

    // find updated data in the current connections
    for(int f = 0; f <= max_fd; f++) {
      if (FD_ISSET(f, &client_fds)) { // found
        if (f == listener) {
          // handle new connections
          socklen_t addr_size = sizeof client_addr;
          client_fd = accept(listener, (struct sockaddr *)&client_addr, &addr_size);

          if (client_fd == -1) {
            perror("server: accept");
            continue;
          } 
          // new user connect
          else {
            // get client ip and port
            char ip_str[NI_MAXHOST];
            char port_str[NI_MAXSERV];
            int new_user_id;

            if (getnameinfo((struct sockaddr *)&client_addr, 
                addr_size, ip_str, sizeof(ip_str), port_str, sizeof(port_str), 
                NI_NUMERICHOST | NI_NUMERICSERV) == 0){
                printf("select_server: got connection from %s:%s\n", ip_str, port_str);
                new_user_id = add_user(ip_str, port_str, client_fd);
            }

            // add new client to server_fd
            FD_SET(client_fd, &server_fd);

            // keep track of the max fd
            if (client_fd > max_fd) {
              max_fd = client_fd;
            }

            // add user env
            UserEnv ue;
            ue.id = new_user_id;
            ue.env_var["PATH"] = "bin:.";
            user_env.push_back(ue);

            // show welcome msg
            std::string welcome_msg = 
            "****************************************\n"
            "** Welcome to the information server. **\n"
            "****************************************\n";
            send(client_fd, welcome_msg.c_str(), welcome_msg.size(), 0);

            // broadcast
            std::string login_msg;
            login_msg = "*** User '(no name)' entered from " + std::string(ip_str) + \
                        ":" + std::string(port_str) + ". ***\n";
            broadcast(login_msg);
            
            // first prompt
            send(client_fd, "% ", 2, 0);
          }
        } 

        // client main
        else {
          // connection close
          int recv_status = (user_input_size = recv(f, client_input, sizeof client_input, 0));
          if (recv_status <= 0) {
            // got error or connection closed by client
            if (user_input_size == 0) {
              printf("server: socket %d left up\n", f);
              std::string left_msg = \
                "*** User '" + std::string(get_user_by_fd(f).name) + "' left. ***\n";
              broadcast(left_msg);
            } 
            else {
              perror("server: recv error");
            }
            // close connection
            close(f);
            // remove from server fd list
            FD_CLR(f, &server_fd);
            remove_user(get_user_by_fd(f).id);
          }
          
          // get input from user
          else{

            std::string usr_input(client_input);
            memset(client_input, '\0', sizeof(client_input));

            // remove all \r \n in the input
            usr_input.erase(std::remove(usr_input.begin(), usr_input.end(), '\r'), usr_input.end());
            usr_input.erase(std::remove(usr_input.begin(), usr_input.end(), '\n'), usr_input.end());
            
            // save current stdout
            int save_stdout = dup(STDOUT_FILENO);
            int save_stderr = dup(STDERR_FILENO);
            dup2(f, STDOUT_FILENO);
            dup2(f, STDERR_FILENO);

            // set user env
            int user_id = get_user_by_fd(f).id;
            set_user_env(user_id);

            // execute user
            ConnectInfo info = {
                .id = user_id,
                .user_table = user_table,
                .usr_input = usr_input
            };
            int status = run_cmd(usr_input, info);

            // prompt
            if (status == SUCCESS){
              send(f, "% ", 2, 0);
            }
            // user exit, close pipe
            else{ 
              std::string left_msg = \
                "*** User '" + std::string(get_user_by_fd(f).name) + "' left. ***\n";
              broadcast(left_msg);
              close(f);
              remove_user(user_id);
              FD_CLR(f, &server_fd);
            }
            
            // Restore stdout/err & env
            dup2(save_stdout, STDOUT_FILENO);
            dup2(save_stderr, STDERR_FILENO);
            restore_user_env(user_id);
          }         
        }
      }
    }
  }

  return 0;
}
