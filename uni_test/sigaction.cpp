#include <iostream>
#include <csignal>
#include <unistd.h>

using namespace std;

void signalHandler(int signo, siginfo_t *info, void *extra) {
   int int_val = info->si_value.sival_int;
   printf("Signal: %d, value: [%d]\n", signo, int_val);
}

int main () {
   int i = 0;
   // register signal SIGINT and signal handler  
   //    signal(SIGUSR1, signalHandler);  

   // regist sig handler
   struct sigaction action;
   action.sa_flags = SA_SIGINFO;
   action.sa_sigaction = &signalHandler;
   sigaction(SIGUSR1, &action, NULL);

   union sigval value;
   pid_t pid = getpid();


   for (size_t i = 0; i < 10; i++){
      cout << "Going to sleep...." << endl;
      
      if( i == 3 ) {
         value.sival_int = i;
         sigqueue(pid, SIGUSR1, value);
        //  raise(SIGUSR1);
      }
      if( i == 5 ) {
        value.sival_int = i;
         sigqueue(pid, SIGUSR1, value);
        //  raise(SIGUSR1);
      }
      sleep(1);
   }

    value.sival_int = -1;
    sigqueue(pid, SIGUSR1, value);



   return 0;
}