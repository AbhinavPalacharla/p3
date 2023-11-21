CC = gcc
CFLAGS = -g -W -Wall

# all: part1

part2: main.o account.o transaction.o utils.o
	$(CC) $(CFLAGS) -o part2 main.o account.o transaction.o utils.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

transaction.o: transaction.c
	$(CC) $(CFLAGS) -c transaction.c

account.o: account.c
	$(CC) $(CFLAGS) -c account.c

utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o part2