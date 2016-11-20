CFLAGS=  -ansi  -Wall -g -pedantic-errors -std=c99 -D_GNU_SOURCE

all: mail_server client

mail_server: mail_server.o message.o
	gcc $(CFLAGS) mail_server.o message.o -o mail_server

client: client.o message.o
	gcc $(CFLAGS) client.o message.o -o client

mail_server.o:	 mail_server.c message.h nw.h
	gcc $(CFLAGS) -g -c mail_server.c

client.o: client.c nw.h message.h
	gcc $(CFLAGS) -g -c client.c

message.o: message.c nw.h
	gcc $(CFLAGS) -g -c message.c	



clean:
	rm -f *.o mail_server client
	