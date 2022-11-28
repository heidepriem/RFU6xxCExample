main: open62541.o main.o RFU6xxClient.o
	gcc open62541.o main.o RFU6xxClient.o -o main

open62541.o: open62541.c
	gcc -c -std=c99 open62541.c -o open62541.o

rfu6xxClient.o: RFU6xxClient.c
	gcc -c RFU6xxClient.c -o RFU6xxClient.o

main.o: main.c
	gcc -c main.c

clean:
	rm *.o main

run:
	./main