main.out: main.o loader.o hashmap.o
	gcc -o main.out main.o loader.o hashmap.o
hashmap.o: hashmap.c hashmap.h
	gcc -c hashmap.c
loader.o: loader.c loader.h hashmap.h
	gcc -c loader.c
main.o: main.c hashmap.h loader.h
	gcc -c main.c
clean:
	rm -f main.out *.o