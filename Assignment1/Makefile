HEADERS = minishell.h

default: minishell

minishell.o: minishell.c $(HEADERS)
	    gcc -g -c minishell.c -o msh.o

minishell: minishell.o
	    gcc msh.o -o msh

clean:
	-rm -f msh.o
	-rm -f msh
