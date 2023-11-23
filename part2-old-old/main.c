#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"
#include "transaction.h"
#include "utils.h"
#include <unistd.h>

#define THREAD_POOL_SIZE 5
#define NUM_TRANSACTIONS_PER_THREAD 3
// #define THREAD_POOL_SIZE 10
// #define NUM_TRANSACTIONS_PER_THREAD 12000

typedef struct _ThreadHandlerArgs {
    TransactionQueue *tq;
    account *accounts;
    int num_accounts;
} ThreadHandlerArgs;

void *thread_handler(void *arg) {
    printf("Thread Started...\n");
    ThreadHandlerArgs *args = (ThreadHandlerArgs *) arg;

    while(true) {
        Transaction *t = args->tq->dequeue(args->tq);

        if(t == NULL) { printf("#%d FINISHED", pthread_self()); break; }

        printf("T# %d | TRANS [%d/%d]\n", pthread_self(), (NUM_TRANSACTIONS_PER_THREAD - args->tq->size), NUM_TRANSACTIONS_PER_THREAD);
        printf("T# %d | ", pthread_self()); view_transaction(t);
        handle_transaction(t, args->accounts, args->num_accounts);
    }
}

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

    /*THREAD POOL*/
    pthread_t thread_pool[THREAD_POOL_SIZE];

    /*TRANSACTIONS*/
    TransactionQueue *tq = init_transaction_queue();

    size_t len = 128; char *line = malloc(sizeof(char) * len);  ssize_t read;

    //read transactions
    while((read = getline(&line, &len, f)) != -1) {
        tq->enqueue(tq, read_transaction(line));
    }

    free(line);

    //divide transactions
    int num_thread_transactions = tq->size / THREAD_POOL_SIZE;
    printf("num_thread_transactions: %d\n", num_thread_transactions);
    TransactionQueue *tqs[THREAD_POOL_SIZE];

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        tqs[i] = init_transaction_queue();
    }

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        for(int j = 0; j < num_thread_transactions; j++) {
            Transaction *t = tq->dequeue(tq);
            t->next = NULL;
            tqs[i]->enqueue(tqs[i], t);
            // tqs[i]->enqueue(tqs[i], tq->dequeue(tq));
        }
    }

    free_transactions_queue(tq);

    // view_transactions_queue(tqs[0]);

    // for(int i = 0; i < THREAD_POOL_SIZE; i++) {
    //     view_transactions_queue(tqs[i]);
    // }

    // populate thread pool
    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        ThreadHandlerArgs *args = (ThreadHandlerArgs *) malloc(sizeof(ThreadHandlerArgs));
        
        args->tq = tqs[i];
        args->accounts = accounts;
        args->num_accounts = num_accounts;

        pthread_create(&thread_pool[i], NULL, thread_handler, (void *) args);
    }

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(thread_pool[i], NULL);
    }

    issue_reward(accounts, num_accounts);

    printf("\n");

    view_accounts(accounts, num_accounts);
    // free_accounts(accounts, num_accounts);

    // for(int i = 0; i < THREAD_POOL_SIZE; i++) {
    //     free_transactions_queue(tqs[i]);
    // }

    // sleep(100);

    // for(int i = 0; i < 100; i++) {
    //     printf("HERE\n");
    // }

    fclose(f);

    return 0;
}
