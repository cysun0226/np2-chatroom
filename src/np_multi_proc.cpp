#include "../include/np_multi_proc.h"
#include "../include/npshell_multi.h" 
#include "../include/server.h"

// variables ---------------------------------------------------------------------
User* user_table;
int shm_id;
char* broadcast_buf;
int broadcast_shm_id;
pid_t main_pid;
int tell_shm_id;
char* tell_buf;

// functions ---------------------------------------------------------------------
void create_shm(){
    // locate the share memory
    shm_id = shmget(IPC_PRIVATE, (MAX_USER_NUM+10) * sizeof(User), IPC_CREAT | 0600);
    if (shm_id < 0){
        printf("shmget error");
        exit(-1);
    }
    broadcast_shm_id = shmget(IPC_PRIVATE, 200 * sizeof(char), IPC_CREAT | 0600);
    if (broadcast_shm_id < 0){
        printf("broadcast shmget error");
        exit(-1);
    }
    tell_shm_id = shmget(IPC_PRIVATE, ((MAX_USER_NUM+1)*MAX_TELL_LENGTH) * sizeof(char), IPC_CREAT | 0600);
    if (tell_shm_id < 0){
        printf("tell shmget error");
        exit(-1);
    }

    // attach the shared memory
    user_table = (User*)shmat(shm_id, NULL, 0);
    broadcast_buf = (char*)shmat(broadcast_shm_id, NULL, 0);
    tell_buf = (char*)shmat(tell_shm_id, NULL, 0);
    memset(broadcast_buf, 0, 200);
    memset(tell_buf, 0, (MAX_USER_NUM+1)*MAX_TELL_LENGTH);
}

void close_handler(int s) {
    std::cout << "\nserver close..." << std::endl;
    shmdt(user_table);
    shmdt(broadcast_buf);
    shmdt(tell_buf);
    shmctl(shm_id, IPC_RMID, NULL);
    shmctl(tell_shm_id, IPC_RMID, NULL);
    shmctl(broadcast_shm_id, IPC_RMID, NULL);
    
    DIR* dp;
    struct dirent* ep;
    std::string path = "./user_pipe/";
    dp = opendir(path.c_str());
    if (dp != NULL){
        while (ep = readdir(dp)){
            if (ep->d_type == DT_FIFO){
                std::string file_name(ep->d_name);
                bool to_delete = true;
                // prevent to delete nfs node
                for (size_t i = 0; i < file_name.size(); i++){
                    if (std::isdigit(file_name.c_str()[i]) != true){
                        to_delete = false;
                        break;
                    }
                }
                if (to_delete != true){
                    continue;   
                }

                printf("remove %s\n", ep->d_name);
                
                file_name = "./user_pipe/" + file_name;
                remove(file_name.c_str());
            }   
        }
    }

    exit(0);
}

void remove_user_handler(int s) {
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id == -1 && user_table[i].clear != true){
            std::cout << "remove user " << i << std::endl;
            close(user_table[i].fd);
            user_table[i].clear = true;
        }
    }
}

void broadcast(std::string msg){
    // raise(SIGUSR1);
    msg.pop_back();
    strcpy(broadcast_buf, msg.c_str());

    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id != -1){
            kill(user_table[i].pid, SIGUSR1);
        }
    }
}

void send_broadcast(){
    std::string msg(broadcast_buf);
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id != -1){
            send(user_table[i].fd, msg.c_str(), msg.length(), 0);
        }
    }
}

void receive_broadcast(int signum) {
   std::string msg(broadcast_buf);
   std::cout << broadcast_buf << std::endl;
}

void receive_msg(int sig, siginfo_t *info, void *extra) {
   int act = info->si_value.sival_int;

   // tell
   if (act >= 10000){
       int to = act % 100;
       int from = ((act - to) / 100) % 100;
       User u = get_user(to, user_table);
       std::cout << "from " << from << " to " << to << std::endl;
       send(u.fd, tell_buf, std::string(tell_buf).size(), 0);
       return;
   }

   return;
}

void close_other_pipe(int id, User* user_table){
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id != id){
            close(user_table[i].fd);
        }
    }
}

void init_user_table(User* user_table){
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        user_table[i].id = -1;
        user_table[i].clear = true;
    }
}

User get_user(int id, User* user_table){
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

int add_user(User* user_table, char* ip, char* port, int client_fd){
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

    return new_user_id;
}

void update_user_table(int id, pid_t pid, int fd, User* user_table){
    if (pid == 0){
        return;
    }
    
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id == id){
            user_table[i].pid = pid;
            user_table[i].fd = fd;
            user_table[i].clear = false;
            return;
        }
    }
}

void remove_user(User* user_table, int id){
    // std::cout << "remove user " << id << std::endl;
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        // std::cout << "user[i].id = " << user_table[i].id << " id = " << id << std::endl;
        if (user_table[i].id == id){
            user_table[i].id = -1;
            break;
        }
    }

    // clean up non-read pipe
    DIR* dp;
    struct dirent* ep;
    std::string path = "./user_pipe/";
    dp = opendir(path.c_str());
    if (dp != NULL){
        while (ep = readdir(dp)){
            if (ep->d_type == DT_FIFO){
                std::string file_name(ep->d_name);
                
                // check if is nfs file
                bool to_delete = true;
                for (size_t i = 0; i < file_name.size(); i++){
                    if (std::isdigit(file_name.c_str()[i]) != true){
                        to_delete = false;
                        break;
                    }
                }
                if (to_delete != true){
                    continue;   
                }

                file_name = "./user_pipe/" + file_name;
                // std::cout << "remove " << file_name << std::endl;
                int from = std::stoi(file_name.substr(12, 2));
                int to = std::stoi(file_name.substr(14, 2));
                if (to == id || from == id){
                    remove(file_name.c_str());
                }
            }   
        }
    }

}

int create_named_pipe(int from, int to){
    // FIFO file path 
    std::string fifo_path = "./user_pipe/"; 
    // std::cout << "from = " << from << ", to = " << to << std::endl;

    // create named pipe
    // named_pipe format: [from][to]
    // e.g. ./user_pipe/0113
    int digit = 2;
    std::string f_str = std::string(digit - std::to_string(from).length(), '0') + std::to_string(from);
    std::string t_str = std::string(digit - std::to_string(to).length(), '0') + std::to_string(to);
    std::string fifo_name = fifo_path + f_str + t_str;
    int status =  mkfifo(fifo_name.c_str(), 0666); 
    return status;
}

int remove_named_pipe(int from, int to){
    // FIFO file path 
    std::string fifo_path = "./user_pipe/"; 
    // std::cout << "from = " << from << ", to = " << to << std::endl;

    // remove named pipe
    // named_pipe format: [from][to]
    // e.g. ./user_pipe/0113
    int digit = 2;
    std::string f_str = std::string(digit - std::to_string(from).length(), '0') + std::to_string(from);
    std::string t_str = std::string(digit - std::to_string(to).length(), '0') + std::to_string(to);
    std::string fifo_name = fifo_path + f_str + t_str;
    int status = remove(fifo_name.c_str());
    return status;
}

// main ---------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // check if input port
    if (argc < 2) {
        std::cout << "please input port!" << std::endl;
        exit(0);
    }
    std::string port = argv[1];
    
    // regist the ctrl-c exit
    signal(SIGINT, close_handler);
    signal(SIGUSR1, remove_user_handler);
    main_pid = getpid();

    // regist broadcast/tell handler
    struct sigaction action;
    action.sa_flags = SA_SIGINFO;
    // action.sa_flags = SA_RESTART;
    action.sa_sigaction = &receive_msg;
    sigaction(SIGUSR2, &action, NULL);

    // create share memory
    create_shm();
    init_user_table(user_table);

    // launch server
    int client_fd, listen_fd = launch_server(port);
    std::cout << "wait for connection..." << std::endl;

    // loop of accept
    socklen_t addr_size;
    struct sockaddr_storage client_addr;
    char ip_str[NI_MAXHOST];
    char port_str[NI_MAXSERV];

    while (1){
        addr_size = sizeof client_addr;

        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_size);

        if (client_fd < 0){
            perror("server: open accept fd failed");
            continue;
        }
        
        // get client ip and port
        if (getnameinfo((struct sockaddr *)&client_addr, 
            addr_size, ip_str, sizeof(ip_str), port_str, sizeof(port_str), 
            NI_NUMERICHOST | NI_NUMERICSERV) == 0){
            printf("server: got connection from %s:%s\n", ip_str, port_str);
        }
        else {
            printf("server: can't get client ip and port\n");
        }

        // add user to user_table
        int user_id = add_user(user_table, ip_str, port_str, client_fd);

        // show welcome message
        std::string welcome_msg = 
        "****************************************\n"
        "** Welcome to the information server. **\n"
        "****************************************\n";
        send(client_fd, welcome_msg.c_str(), welcome_msg.size(), 0);

        // broadcast login
        std::string login_msg;
        login_msg = "*** User '(no name)' entered from " + std::string(ip_str) + \
                        ":" + std::string(port_str) + ". ***\n";
        strcpy(broadcast_buf, login_msg.c_str());
        send_broadcast();

        // fork to handle connection
        pid_t pid = fork();
        update_user_table(user_id, pid, client_fd, user_table);

        if (pid == 0) // child process
        {
            close(listen_fd); // child does not need listener
            close_other_pipe(user_id, user_table);
            close(STDOUT_FILENO);
            close(STDIN_FILENO);
            close(STDERR_FILENO);

            signal(SIGUSR1, receive_broadcast);

            if (dup(client_fd) != STDIN_FILENO || dup(client_fd) != STDOUT_FILENO || dup(client_fd) != STDERR_FILENO){
                std::cout << "can't dup socket for stdin/out/err" << std::endl;
                exit(1);
            }

            // execute shell
            user_table = (User*)shmat(shm_id, NULL, 0);
            broadcast_buf = (char*)shmat(broadcast_shm_id, NULL, 0);
            tell_buf = (char*)shmat(tell_shm_id, NULL, 0);
            ConnectInfo info = {
                .id = user_id,
                .user_table = user_table,
                .broadcast_buf = broadcast_buf,
                .tell_buf = tell_buf
            };
            
            npshell(info);

            // user left
            std::string left_msg = \
            "*** User '" + std::string(get_user(user_id, user_table).name) + "' left. ***";

            std::cout << left_msg << std::endl;

            close(client_fd);
            remove_user(user_table, user_id);
            
            broadcast(left_msg+"\n");
            kill(getppid(), SIGUSR1);
            
            // detach
            shmdt(user_table);
            shmdt(broadcast_buf);
            shmdt(tell_buf);

            exit(0);
        }                
    }

    return 0;
}