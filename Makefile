CC=gcc
CFLAGS=-I.

client: client.c requests.c helpers.c buffer.c
	$(CC) -o client parson.c client.c requests.c helpers.c buffer.c -Wall

run: client
	./client

clean:
	rm -f *.o client
