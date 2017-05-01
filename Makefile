TARGET: server client

CC	= g++
CFLAGS	= -Wall -O2
LFLAGS	= -Wall

server.o client.o err.o: err.h

server: server.o err.o
	$(CC) $(LFLAGS) $^ -o $@

client: client.o err.o
	$(CC) $(LFLAGS) $^ -o $@

.PHONY: clean TARGET
clean:
	rm -f server client *.o *~ *.bak
