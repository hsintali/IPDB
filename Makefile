all: server client main
	rm -f *.o

server: ../kafka/librdkafka/src/librdkafka.a socket_server.c hashmap.c loader.c ipdb_protocol.c lpm.c
	gcc socket_server.c hashmap.c loader.c ipdb_protocol.c -o server.out lpm.c \
		../kafka/librdkafka/src/librdkafka.a -lm -ldl -lpthread -lrt

client: socket_client.c ipdb_protocol.c
	gcc -o client.out socket_client.c ipdb_protocol.c

main: main.c lpm.c loader.c hashmap.c
	gcc -o main.out main.c lpm.c loader.c hashmap.c

clean:
	rm -f *.out *.o