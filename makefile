CC=gcc
CFLAGS = -g

# Put header files in DEPS to allow make to rebuild .o files when header files are changed
DEPS = connection.h message.h sessions.h clientInfo.h linkedList.h
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
all: server client


server: server.o message.o connection.o
	$(CC) -o server server.o message.o connection.o $(CFLAGS)
client: client.o message.o connection.o
	$(CC) -o client -pthread client.o message.o connection.o $(CFLAGS)
clean:
	rm -f *.o server client 
