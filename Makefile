all: server.out client.out
client.out: socket_client.o
	gcc -o client.out socket_client.o
	rm -f *.o
client.o: socket_client.c socket_client.h
	gcc -c socket_client.c
server.out: socket_server.o loader.o hashmap.o
	gcc -o server.out socket_server.o loader.o hashmap.o -lpthread
	rm -f *.o
server.o: socket_server.c socket_server.h
	gcc -c socket_server.c -lpthread
hashmap.o: hashmap.c hashmap.h
	gcc -c hashmap.c
loader.o: loader.c loader.h hashmap.h
	gcc -c loader.c
clean:
	rm -f *.out *.o