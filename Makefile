all: server.out client.out
	rm -f *.o
client.out: socket_client.o ipdb_protocol.o
	gcc -o client.out socket_client.o ipdb_protocol.o
client.o: socket_client.c socket_client.h
	gcc -c socket_client.c
server.out: socket_server.o loader.o hashmap.o ipdb_protocol.o
	gcc -o server.out socket_server.o loader.o hashmap.o ipdb_protocol.o -lpthread
server.o: socket_server.c socket_server.h
	gcc -c socket_server.c -lpthread
hashmap.o: hashmap.c hashmap.h
	gcc -c hashmap.c
loader.o: loader.c loader.h hashmap.h
	gcc -c loader.c
protocol.o: ipdb_protocol.c ipdb_protocol.h
	gcc -c ipdb_protocol.c
clean:
	rm -f *.out *.o