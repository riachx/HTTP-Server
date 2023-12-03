CC = clang
CFLAGS = -Wall -Wpedantic -Werror -Wextra

all: httpserver

httpserver: httpserver.o parser.o asgn2_helper_funcs.a
	$(CC) -o httpserver httpserver.o parser.o asgn2_helper_funcs.a

httpserver.o: httpserver.c parser.c asgn2_helper_funcs.h
	$(CC) $(CFLAGS) -c httpserver.c parser.c 

clean:
	rm -f httpserver *.o *.pdf

format:
	clang-format -i -style=file httpserver.c parser.c