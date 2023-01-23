all: server client

server: PPS.c PPS.h
	gcc -pthread -Wall -o PPS PPS.c

client: PQC.c PQC.h
	gcc -pthread -Wall -o PQC PQC.c
