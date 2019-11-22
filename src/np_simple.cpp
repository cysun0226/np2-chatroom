#include "../include/np_simple.h"
#include "../include/npshell.h" 


// variables ---------------------------------------------------------------------

// functions ---------------------------------------------------------------------
void close_handler(int s){
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
    std::string port = argv[1];
    
    // regist the ctrl-c exit
    signal(SIGINT, close_handler);

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
            perror("server: accept fd failed");
            continue;
        }
        
        // get client ip and port
        if (getnameinfo((struct sockaddr *)&client_addr, 
            addr_size, ip_str, sizeof(ip_str), port_str, sizeof(port_str), 
            NI_NUMERICHOST | NI_NUMERICSERV) == 0){
            printf("server: got connection from %s:%s, fd=%d\n", ip_str, port_str, client_fd);
        }
        else {
            printf("server: can't get client ip and port\n");
        }

        // fork to handle connection
        pid_t pid = fork();

        if (pid == 0) // child process
        {
            close(listen_fd); // child does not need listener
            
            close(STDOUT_FILENO);
            close(STDIN_FILENO);
            close(STDERR_FILENO);

            if (dup(client_fd) != STDIN_FILENO || dup(client_fd) != STDOUT_FILENO || dup(client_fd) != STDERR_FILENO){
                std::cerr << "can't dup socket to stdin/out/err" << std::endl;
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