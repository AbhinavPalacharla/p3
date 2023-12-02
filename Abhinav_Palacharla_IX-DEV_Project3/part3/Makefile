CC = gcc
CFLAGS = -g -W -Wall -pthread -lpthread

# all: part1

bank: bank.o account.o transaction.o utils.o worker_thread.o bank_thread.o
	mkdir output
	$(CC) $(CFLAGS) -o bank bank.o account.o transaction.o utils.o worker_thread.o bank_thread.o -lpthread

bank.o: bank.c
	$(CC) $(CFLAGS) -c bank.c

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
	rm -f *.o bank act_*
	rm -rf output