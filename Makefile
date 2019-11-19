simple_obj = ./src/np_simple.o ./src/npshell.o ./src/parse.o ./src/execute.o

np_simple : $(simple_obj)
	g++ -g -o np_simple $(simple_obj)

./src/np_simple.o : ./include/np_simple.h
./src/npshell.o : ./include/npshell.h
./src/parse.o : ./include/parse.h
./src/execute.o : ./include/execute.h

.PHONY: clean
clean :
	rm np_simple &(objects)

