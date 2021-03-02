all: server client
	rm -f *.o

server: ../kafka/librdkafka/src/librdkafka.a socket_server.c hashmap.c loader.c ipdb_protocol.c lpm.c
	gcc -o server.out socket_server.c hashmap.c loader.c ipdb_protocol.c lpm.c \
		../kafka/librdkafka/src/librdkafka.a -lm -ldl -lpthread -lrt -lsasl2 -lssl -lcrypto

client: socket_client.c ipdb_protocol.c
	gcc -o client.out socket_client.c ipdb_protocol.c

clean:
	rm -f *.out *.o
