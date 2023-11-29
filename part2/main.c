#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"
#include "transaction.h"
#include "utils.h"
#include <unistd.h>
#include "worker_thread.h"
#include "bank_thread.h"
#include <pthread.h>

pthread_t bank_thread;

int main(int argc, char **argv) {
    FILE *f;
    
    if((f = fopen(argv[1], "r")) == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    /*ACCOUNTS*/
    int num_accounts = read_num_accounts(f);

    account *accounts = (account *) malloc(sizeof(account) * num_accounts); //array of accounts

    read_accounts(accounts, f, num_accounts); //populate accounts

    view_accounts(accounts, num_accounts);

    /*TRANSACTIONS*/
    TransactionQueue *tq = init_transaction_queue();

    size_t len = 128; char *line = malloc(sizeof(char) * len);  ssize_t read;

    while((read = getline(&line, &len, f)) != -1) {
        tq->enqueue(tq, read_transaction(line));
    }

    free(line);

    /*WORKER THREADS*/
    init_worker_threads(tq, accounts, num_accounts);

    /*BANK THREAD*/
    init_bank_thread(accounts, num_accounts);

    /*THE END*/
    printf("\n");

    view_accounts(accounts, num_accounts);

    fclose(f);

    return 0;
}
