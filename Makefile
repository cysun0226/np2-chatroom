.DEFAULT_GOAL:=all

all : np_simple np_single_proc np_multi_proc

simple_obj = ./src/np_simple.o ./src/npshell.o ./src/parse.o ./src/execute.o ./src/server.o
single_obj = ./src/np_single_proc.o ./src/npshell_single.o ./src/parse_single.o ./src/execute_single.o
multi_obj = ./src/np_multi_proc.o ./src/npshell_multi.o ./src/parse_multi.o ./src/execute_multi.o ./src/server.o

np_simple : $(simple_obj)
	g++ -std=c++11 -g -o np_simple $(simple_obj)

np_single_proc : $(single_obj)
	g++ -std=c++11 -g -o np_single_proc $(single_obj)

np_multi_proc : $(multi_obj)
	g++ -std=c++11 -g -o np_multi_proc $(multi_obj)


./src/server.o : ./include/server.h

./src/np_simple.o : ./include/np_simple.h
./src/npshell.o : ./include/npshell.h
./src/parse.o : ./include/parse.h
./src/execute.o : ./include/execute.h

./src/np_single_proc.o : ./include/np_single_proc.h
./src/npshell_single.o : ./include/npshell_single.h
./src/parse_single.o : ./include/parse_single.h
./src/execute_single.o : ./include/execute_single.h

./src/np_multi_proc.o : ./include/np_multi_proc.h
./src/npshell_multi.o : ./include/npshell_multi.h
./src/parse_multi.o : ./include/parse_multi.h
./src/execute_multi.o : ./include/execute_multi.h

.PHONY: clean
clean :
	rm -f np_simple
	rm -f np_single_proc
	rm -f np_multi_proc
	rm -f  ./src/*.o

