#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"
#include "transaction.h"
#include "utils.h"
#include <unistd.h>
#include "worker_thread.h"

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
    WorkerThread *wts = init_worker_threads(tq, accounts, num_accounts);

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(wts[i].thread, NULL);
    }

    free_transactions_queue(tq);

    /*REWARD*/
    issue_reward(accounts, num_accounts);

    /*THE END*/
    printf("\n");

    view_accounts(accounts, num_accounts);

    fclose(f);

    return 0;
}
