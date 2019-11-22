#include "../include/np_simple.h"
#include "../include/npshell.h" 


// variables ---------------------------------------------------------------------

// functions ---------------------------------------------------------------------
void fork_handler(int s){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void close_handler(int s) {
    std::cout << "\nserver close..." << std::endl;
    exit(0);
}

// main ---------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // check if input port
    if (argc < 2) {
        std::cout << "please input port!" << std::endl;
        exit(0);
    }
    std::string PORT = argv[1];
    
    // regist the ctrl-c exit
    signal(SIGINT, close_handler);

    // hints: specify the expected return type
    struct addrinfo hints;

    // server_info: a pointer that point to a addrinfo
    struct addrinfo *server_info;
    memset(&hints, 0, sizeof hints);
    
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // use address in bind (listen)

    // build a socket, and bind to the given port
    int get_addr_info_status;
    if ((get_addr_info_status = getaddrinfo(NULL, PORT.c_str(), &hints, &server_info)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_addr_info_status));
        return 1;
    }


    int sockfd, client_fd;
    struct addrinfo *p = server_info;
    struct sigaction sa; // signal action
    int yes = 1;

    

    /* bind to the first available socket */
    // for(p = server_info; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            perror("server: can't open socket");
            // continue;
        }
        // set SO_REUSEADDR
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
            perror("server: can't set socket option");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            perror("server: bind socket failed");
            // continue;
        }
        // break;
    // }

    if (p == NULL) {
        fprintf(stderr, "server: no available port\n");
        exit(1);
    }

    freeaddrinfo(server_info); // release the used addrinfo

    // listen to the socket
    if (listen(sockfd, MAX_USER_NUM) < 0) {
        perror("server: can't listen");
        exit(1);
    }

    sa.sa_handler = fork_handler; // deal with the died child
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
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd < 0){
            perror("server: open accept fd failed");
        }

        if (client_fd < 0) {
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

        // fork to handle connection
        std::cout << "client_fd = " << client_fd << std::endl;
        pid_t pid = fork();
        
        if (pid == 0) // child process
        {
            close(sockfd); // child does not need listener
            
            close(STDOUT_FILENO);
            close(STDIN_FILENO);
            close(STDERR_FILENO);

            if (dup(client_fd) != STDIN_FILENO || dup(client_fd) != STDOUT_FILENO || dup(client_fd) != STDERR_FILENO){
                std::cout << "can't dup socket to stdin/out/err" << std::endl;
                exit(1);
            }

            // execute shell
            npshell();

            close(client_fd);
            exit(0);
        }        

        close(client_fd);        
    }

    return 0;

}