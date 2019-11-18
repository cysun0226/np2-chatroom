#include "../include/server.h"
#include "../include/npshell.h" 


// variables ---------------------------------------------------------------------
// std::vector<User> user_table;
// std::vector<User>* user_table;
User* user_table;
int shm_id;
char* broadcast_buf;
int broadcast_shm_id;
bool broadcast_flag;
pid_t main_pid;
int current_login;
int tell_shm_id;
char* tell_buf;
bool tell_table[MAX_USER_NUM+1][MAX_USER_NUM+1];
// char tell_msg_buf[MAX_USER_NUM+1][MAX_TELL_LENGTH];

// functions ---------------------------------------------------------------------
void close_handler(int s) {
    std::cout << "server close..." << std::endl;
    shmdt(user_table);
    shmctl(shm_id, IPC_RMID, NULL);
    
    DIR* dp;
    struct dirent* ep;
    char* path = "./user_pipe/";
    dp = opendir(path);
    if (dp != NULL){
        while (ep = readdir(dp)){
            if (ep->d_type == DT_FIFO){
                printf("remove %s\n", ep->d_name);
                remove(ep->d_name);
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
    // std::cout << msg << std::endl;
    // raise(SIGUSR1);
    strcpy(broadcast_buf, msg.c_str());
    
    union sigval value;
    value.sival_int = BROADCAST_SIG;
    sigqueue(main_pid, SIGUSR2, value);

    // for (size_t i = 0; i < MAX_USER_NUM; i++){
    //     if (user_table[i].id != -1){
    //         // kill(user_table[i].pid, SIGUSR1);
    //         union sigval value;
    //         value.sival_int = BORADCAST_SIG;
    //         std::cout << "user_table[" << i << "].pid = " << user_table[i].pid << std::endl;
    //         sigqueue(user_table[i].pid, SIGUSR1, value);
    //     }
    // }

    // for (size_t i = 0; i < MAX_USER_NUM; i++){
    //     if (user_table[i].id != -1){
    //         std::cout << "user_table[" << i << "].fd = " << user_table[i].fd << std::endl;
    //         send(user_table[i].fd, msg.c_str(), msg.size(), 0);
    //     }
    // }
}



void send_broadcast(){
    std::string msg(broadcast_buf);
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (user_table[i].id != -1){
            send(user_table[i].fd, msg.c_str(), msg.size(), 0);
        }
    }
    broadcast_flag = false;
}

void sig_handler(int s){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

User get_user_by_name(std::string name){
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (std::string(user_table[i].name) == name){
            return user_table[i];
        }
    }
    User err;
    return err;
}

void receive_broadcast(int signum) {
   std::cout << broadcast_buf << std::endl;
   std::string msg(broadcast_buf);
   
   // if user exit
//    if (msg.find("left") != std::string::npos) {
//        size_t quot_1 = msg.find("'");
//        size_t quot_2 = msg.find("'", quot_1);
//        std::string name = msg.substr(quot_1+1, quot_2);
//        std::cout << name << std::endl;
//        for (size_t i = 0; i < MAX_USER_NUM; i++){
//             if (user_table[i].id == -1 && std::string(user_table[i].name) == name){
//                 std::cout << "remove user " << i << std::endl;
//                 close(user_table[i].fd);
//                 // user_table[i].clear = true;
//             }
//        }
//    }
}

void receive_msg(int sig, siginfo_t *info, void *extra) {
   int act = info->si_value.sival_int;
   
   // broadcast
   if (act == BROADCAST_SIG){
       send_broadcast();
       return;
   }

   // tell
   if (act >= 10000){
       int to = act % 100;
       int from = ((act - to) / 100) % 100;
       User u = get_user(to, user_table);
       std::cout << "from " << from << "to " << to << std::endl;
       send(u.fd, tell_buf, std::string(tell_buf).size(), 0);
       return;
   }


   // yell from user
    //    else {
    //        /* code */
    //    }
   
//    printf("Signal: %d, value: [%d]\n", sig, act);
    return;
}

void receive_user_pipe_handler(int sig, siginfo_t *info, void *extra) {
   int act = info->si_value.sival_int;

   // tell
   if (act >= 20000){
       int to = act % 100;
       int from = ((act - to) / 100) % 100;
       
    //    std::cout << "open named piped" <<  << std::endl;
       
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

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_port);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_port);
}

void init_user_table(User* user_table){
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        user_table[i].id = -1;
        user_table[i].clear = true;
    }
}

std::string get_user_name(int id, User* user_table){
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        if (id == user_table[i].id){
            return std::string(user_table[i].name);
        }
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

int add_user(User* user_table, char* ip, char* port, int new_fd){
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
            user_table[i].fd = new_fd;
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

int main()
{
    // regist the ctrl-c exit
    signal(SIGINT, close_handler);
    signal(SIGUSR1, remove_user_handler);
    broadcast_flag = false;
    main_pid = getpid();

    // regist broadcast/tell handler
    struct sigaction action;
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = &receive_msg;
    sigaction(SIGUSR2, &action, NULL);

    


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
    init_user_table(user_table);
    memset(broadcast_buf, 0, 200);
    memset(tell_buf, 0, (MAX_USER_NUM+1)*MAX_TELL_LENGTH);
    
    int status;

    int sockfd, new_fd;

    // hints: specify the expected return type
    struct addrinfo hints;
    // servinfo: a pointer that point to a addrinfo
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof hints);

    struct addrinfo *p;
    struct sigaction sa; // signal action
    int yes = 1;
    
    // IPv4 or 6 are both ok
    hints.ai_family = AF_UNSPEC;
    // TCP stream sockets
    hints.ai_socktype = SOCK_STREAM; 
    // use address in bind (listen)
    hints.ai_flags = AI_PASSIVE; 

    std::string PORT = "3490";
    int BACKLOG = 10;

    // build a socket, and bind to the given port
    int get_addr_info_status;
    if ((get_addr_info_status = getaddrinfo(NULL, PORT.c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_addr_info_status));
        return 1;
    }

    /* bind to the first available socket */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            perror("server: can't open socket");
            continue;
        }
        // set SO_REUSEADDR
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
            perror("server: can't set socket option");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            perror("server: bind socket failed");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: no available port\n");
        exit(1);
    }

    freeaddrinfo(servinfo); // release the used addrinfo

    // listen to the socket
    if (listen(sockfd, BACKLOG) < 0) {
        perror("server: can't listen");
        exit(1);
    }

    sa.sa_handler = sig_handler; // deal with the died child
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("server: sigaction error");
        exit(1);
    }

    std::cout << "wait for connection..." << std::endl;

    // loop of accept
    socklen_t addr_size;
    struct sockaddr_storage client_addr;
    char ip_str[NI_MAXHOST];
    char port_str[NI_MAXSERV];
    current_login = -1;

    while (1){
        addr_size = sizeof client_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (new_fd < 0){
            perror("server: open accept fd failed");
        }

        // if (broadcast_flag == true){
        //     send_broadcast();
        // }

        if (new_fd < 0) {
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
        int user_id = add_user(user_table, ip_str, port_str, new_fd);
        current_login = user_id;

        // show welcome message
        std::string welcome_msg = 
        "****************************************\n"
        "** Welcome to the information server. **\n"
        "****************************************\n";
        send(new_fd, welcome_msg.c_str(), welcome_msg.size(), 0);
        // broadcast login
        std::string login_msg;
        login_msg = "*** User '(no name)' entered from " + std::string(ip_str) + \
                        ":" + std::string(port_str) + ". ***\n";
        strcpy(broadcast_buf, login_msg.c_str());
        send_broadcast();

        // fork to handle connection
        std::cout << "new_fd = " << new_fd << std::endl;
        // pid_t ppid = getpid();
        // std::cout << "parent_pid = " << ppid << std::endl;
        pid_t pid = fork();
        update_user_table(user_id, pid, new_fd, user_table);

        if (pid == 0) // child process
        {
            close(sockfd); // child does not need listener
            close_other_pipe(user_id, user_table);

            // signal(SIGUSR1, SIG_IGN);
            // signal(SIGUSR1, receive_broadcast);

            // regist broadcast/tell handler
            // struct sigaction action;
            // action.sa_flags = SA_SIGINFO;
            // action.sa_sigaction = &receive_msg;
            // sigaction(SIGUSR1, &action, NULL);
            
            close(STDOUT_FILENO);
            close(STDIN_FILENO);
            close(STDERR_FILENO);

            if (dup(new_fd) != STDIN_FILENO || dup(new_fd) != STDOUT_FILENO || dup(new_fd) != STDERR_FILENO){
                std::cout << "can't dup socket for stdin/out/err" << std::endl;
                exit(1);
            }

            // dup2(new_fd, STDOUT_FILENO);  // redirect stdout to new_fd

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

            std::string left_msg = \
            "*** User '" + std::string(get_user(user_id, user_table).name) + "' left. ***\n";

            // execlp("bin/npshell", "bin/npshell", (char*) NULL);

            close(new_fd);
            remove_user(user_table, user_id);
            broadcast(left_msg);
            kill(getppid(), SIGUSR1);
            shmdt(user_table);
            shmdt(broadcast_buf);
            shmdt(tell_buf);

            exit(0);
        }        

        // close(new_fd);
        
    }

    return 0;

}