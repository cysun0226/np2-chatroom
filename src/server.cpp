#include "../include/server.h"
#include "../include/npshell.h" 


// variables ---------------------------------------------------------------------
// std::vector<User> user_table;
// std::vector<User>* user_table;

// functions ---------------------------------------------------------------------

void sig_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
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
    }
}

int add_user(User* user_table, char* ip, char* port){
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
            break;
        }
    }

    return new_user_id;
}

void remove_user(User* user_table, int id){
    // std::cout << "remove user " << id << std::endl;
    for (size_t i = 0; i < MAX_USER_NUM; i++){
        // std::cout << "user[i].id = " << user_table[i].id << " id = " << id << std::endl;
        if (user_table[i].id == id){
            // std::cout << "remove user " << id << std::endl;
            user_table[i].id = -1;
            break;
        }
    }
}

// main ---------------------------------------------------------------------

int main()
{
    // locate the user_table share memory
    int shm_id;
    shm_id = shmget(IPC_PRIVATE, (MAX_USER_NUM+10) * sizeof(User), IPC_CREAT | 0600);
    if (shm_id < 0){
        printf("shmget error");
        exit(-1);
    }
    // attach the shared memory
    // user_table = (std::vector<User>*)shmat(shm_id, NULL, 0);
    // User* shm = (std::vector<User>*)shmat(shm_id, NULL, 0);
    
    User* user_table = (User*)shmat(shm_id, NULL, 0);
    init_user_table(user_table);
    
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

    while (1){
        addr_size = sizeof client_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (new_fd < 0){
            perror("server: open accept fd failed");
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
        int user_id = add_user(user_table, ip_str, port_str);
        

        // fork to handle connection
        pid_t pid = fork();

        if (pid == 0) // child process
        {
            close(sockfd); // child does not need listener
            
            close(STDOUT_FILENO);
            close(STDIN_FILENO);
            close(STDERR_FILENO);

            if (dup(new_fd) != STDIN_FILENO || dup(new_fd) != STDOUT_FILENO || dup(new_fd) != STDERR_FILENO){
                std::cout << "can't dup socket for stdin/out/err" << std::endl;
                exit(1);
            }

            // dup2(new_fd, STDOUT_FILENO);  // redirect stdout to new_fd

            // show welcome message
            std::string welcome_msg = 
            "****************************************\n"
            "** Welcome to the information server. **\n"
            "****************************************";
            
            std::cout << welcome_msg << std::endl;

            // execute shell
            user_table = (User*)shmat(shm_id, NULL, 0);
            npshell(user_id, user_table);

            // execlp("bin/npshell", "bin/npshell", (char*) NULL);

            close(new_fd);
            remove_user(user_table, user_id);
            shmdt(user_table);

            exit(0);
        }

        close(new_fd);
        


        
    }

    return 0;

}