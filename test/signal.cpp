#include <iostream>
#include <csignal>
#include <unistd.h>

using namespace std;

void signalHandler( int signum ) {
   cout << "Signal (" << signum << ") received.\n";
}

int main () {
   int i = 0;
   // register signal SIGINT and signal handler  
   signal(SIGUSR1, signalHandler);  

   for (size_t i = 0; i < 10; i++){
      cout << "Going to sleep...." << endl;
      if( i == 3 ) {
         raise(SIGUSR1);
      }
      if( i == 5 ) {
         raise(SIGUSR1);
      }
      sleep(1);
   }

   return 0;
}