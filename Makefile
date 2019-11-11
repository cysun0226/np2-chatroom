SRC_DIR = src
OBJ_DIR = obj

objects = npshell.o parse.o execute.o

npshell : $(objects)
	g++ -g -o npshell $(objects)

npshell.o : npshell.h
parse.o : parse.h
execute.o : execute.h

.PHONY: clean
clean :
	rm npshell &(objects)

