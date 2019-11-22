#include "../include/server.h"
#include "../include/np_multi_proc.h"

void fork_handler(int s){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void register_client_child_handle(){
    // register the fork client child handler
    struct sigaction sa; // signal action
    sa.sa_handler = fork_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
}

int launch_server(std::string port){
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
    if ((get_addr_info_status = getaddrinfo(NULL, port.c_str(), &hints, &server_info)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_addr_info_status));
        return 1;
    }

    if (server_info == NULL) {
        fprintf(stderr, "server: no available port\n");
        exit(1);
    }

    // open socket
    int listen_fd, yes = 1;
    if ((listen_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol)) < 0){
        perror("server: can't open socket");
    }
    // set SO_REUSEADDR
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0){
        perror("server: can't set socket option");
        exit(1);
    }
    // bind
    if (bind(listen_fd, server_info->ai_addr, server_info->ai_addrlen) < 0) {
        close(listen_fd);
        perror("server: bind socket failed");
    }

    // release the used addrinfo
    freeaddrinfo(server_info); 

    // listen to the socket
    if (listen(listen_fd, MAX_USER_NUM) < 0) {
        perror("server: can't listen");
        exit(1);
    }
    
    return listen_fd;
}