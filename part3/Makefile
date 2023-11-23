CC = gcc
CFLAGS = -g -W -Wall

# all: part1

part3: main.o account.o transaction.o utils.o worker_thread.o bank_thread.o
	$(CC) $(CFLAGS) -o part3 main.o account.o transaction.o utils.o worker_thread.o bank_thread.o -lpthread

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

bank_thread.o: bank_thread.c
	$(CC) $(CFLAGS) -c bank_thread.c

worker_thread.o: worker_thread.c
	$(CC) $(CFLAGS) -c worker_thread.c

transaction.o: transaction.c
	$(CC) $(CFLAGS) -c transaction.c

account.o: account.c
	$(CC) $(CFLAGS) -c account.c

utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o part3