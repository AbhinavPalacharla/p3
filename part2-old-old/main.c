#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"
#include "transaction.h"
#include "utils.h"

#define THREAD_POOL_SIZE 10

typedef struct _ThreadHandlerArgs {
    TransactionsQueue *tq;
    account *accounts;
    int num_accounts;
} ThreadHandlerArgs;

void *thread_handler(void *arg) {
    printf("Thread Started...\n");
    ThreadHandlerArgs *args = (ThreadHandlerArgs *) arg;

    while(true) {
        Transaction *t = args->tq->dequeue(args->tq);

        if(t != NULL) {
            view_transaction(t);
            handle_transaction(t, args->accounts, args->num_accounts);
        }
    }
}

int main(int argc, char **argv) {
    FILE *f;
    
    if((f = fopen(argv[1], "r")) == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    /*ACCOUNTS*/
    int num_accounts = get_num_accounts(f);

    account *accounts = (account *) malloc(sizeof(account) * num_accounts); //array of accounts

    read_accounts(accounts, f, num_accounts); //populate accounts

    view_accounts(accounts, num_accounts);

    /*THREAD POOL*/
    pthread_t thread_pool[THREAD_POOL_SIZE];

    /*TRANSACTIONS*/
    TransactionsQueue *tq = init_transactions_queue();

    //populate thread pool
    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        ThreadHandlerArgs *args = (ThreadHandlerArgs *) malloc(sizeof(ThreadHandlerArgs));
        
        args->tq = tq;
        args->accounts = accounts;
        args->num_accounts = num_accounts;

        pthread_create(&thread_pool[i], NULL, thread_handler, (void *) args);
    }

    size_t len = 128; char *line = malloc(sizeof(char) * len);  ssize_t read;

    //read transactions
    while((read = getline(&line, &len, f)) != -1) {
        tq->enqueue(tq, get_transaction(line));
        // printf("Main Enqueued...\n");

        // Transaction *t = tq->dequeue(tq);
        
        // view_transaction(t);

        // handle_transaction(t, accounts, num_accounts);
    }

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(thread_pool[i], NULL);
    }

    issue_reward(accounts, num_accounts);

    printf("\n");

    view_accounts(accounts, num_accounts);

    free(line);
    free_accounts(accounts, num_accounts);
    free_transactions_queue(tq);

    fclose(f);

    return 0;
}
