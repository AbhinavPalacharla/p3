CC = gcc
CFLAGS = -g -W -Wall -pthread -lpthread

# all: part1

bank: bank.o account.o transaction.o utils.o
	$(CC) $(CFLAGS) -o bank bank.o account.o transaction.o utils.o

bank.o: bank.c
	$(CC) $(CFLAGS) -c bank.c

transaction.o: transaction.c
	$(CC) $(CFLAGS) -c transaction.c

account.o: account.c
	$(CC) $(CFLAGS) -c account.c

utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o bank