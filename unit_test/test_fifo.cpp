#include "../include/server.h"


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

void create_named_pipe(){
    // FIFO file path 
    std::string fifo_path = "./user_pipe/named_pipe"; 
    mkfifo(fifo_path.c_str(), 0666); 
}


// main --------------------------------------------------------------------------

int main()
{
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

    // write content into the named_pipe
    create_named_pipe();        
    std::cout << "wait for connection..." << std::endl;


    // loop of accept
    socklen_t addr_size;
    struct sockaddr_storage client_addr;
    char addr_str[INET6_ADDRSTRLEN];
    while (1){
        addr_size = sizeof client_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (new_fd < 0){
            perror("server: open accept fd failed");
        }
        
        // ip to printable
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), addr_str, sizeof addr_str);
        printf("server: got connection from %s\n", addr_str);

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

            // std::cout << "hihi" << std::endl;

            // dup2(new_fd, named_fd);
            int named_fd = open("./user_pipe/named_pipe", O_RDONLY);

            // std::cout << named_fd << std::endl;

            while (1) {
                char tmp;
                if (read(named_fd, &tmp, 1) < 1) break;
                printf("%c", tmp);
            }

            close(named_fd);

            // if (send(new_fd, "Hellllo~\n", 10, 0) < 0){
            //     perror("server: send to client error");
            // }

            close(new_fd);
            exit(0);
        }

        close(new_fd);

        // fork a process to execute bin to write to user pipe
        pid_t pid_write = fork();
        int named_fd = open("./user_pipe/named_pipe", O_WRONLY);
        if (pid_write == 0)
        {
            // close(STDOUT_FILENO);
            // close(STDIN_FILENO);
            // close(STDERR_FILENO);
            dup2(named_fd, STDOUT_FILENO);
            close(named_fd);

            execlp("bin/cat", "bin/cat", "test.html", (char*) NULL);
            exit(1);
            // std::cout << "hiiiii" << std::endl;    
        }

        int status;
        waitpid(pid_write, &status, WNOHANG);
        close(named_fd);

        
        
        // std::cout << "create_named_pipe" << std::endl;
        // int named_fd = open("./user_pipe/named_pipe", O_WRONLY);
        // std::cout << "named_fd" << std::endl;
        // write(named_fd, "XDDDDD\n", 8); 
        // close(named_fd);


        


        
    }

    return 0;

}