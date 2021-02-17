all: server client
	rm -f *.o

server: ../kafka/librdkafka/src/librdkafka.a socket_server.c hashmap.c loader.c ipdb_protocol.c
	gcc socket_server.c hashmap.c loader.c ipdb_protocol.c -o server.out \
		../kafka/librdkafka/src/librdkafka.a -lm -ldl -lpthread -lrt

client: socket_client.c ipdb_protocol.c
	gcc -o client.out socket_client.c ipdb_protocol.c

clean:
	rm -f *.out *.o