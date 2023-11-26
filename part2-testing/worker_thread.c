#include "worker_thread.h"
#include "utils.h"

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

        if(t == NULL) { printf("T# %ld FINISHED", pthread_self()); break; }

        // printf("T# %d | TRANS [%d/%d]\n", pthread_self(), (NUM_TRANSACTIONS_PER_THREAD - args->tq->size), NUM_TRANSACTIONS_PER_THREAD);
        // printf("T# %d | ", pthread_self()); view_transaction(t);
        handle_transaction(t, args->accounts, args->num_accounts);
    }

    return NULL;
}

WorkerThread *init_worker_threads(TransactionQueue *tq, account *accounts, int num_accounts) {
    WorkerThread *wts = (WorkerThread *) malloc(sizeof(WorkerThread) * THREAD_POOL_SIZE);

    int num_thread_transactions = tq->size / THREAD_POOL_SIZE;

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        wts[i].tq = init_transaction_queue();
    }

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        for(int j = 0; j < num_thread_transactions; j++) {
            Transaction *t = tq->dequeue(tq);
            t->next = NULL;
            wts[i].tq->enqueue(wts[i].tq, t);
        }
    }

    //create threads
    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        ThreadHandlerArgs *args = (ThreadHandlerArgs *) malloc(sizeof(ThreadHandlerArgs));
        
        args->tq = wts[i].tq;
        args->accounts = accounts;
        args->num_accounts = num_accounts;

        pthread_create(&wts[i].thread, NULL, thread_handler, (void *) args);
        // pthread_join(wts[i].thread, NULL);
    }

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        // pthread_join(wts[i].thread, NULL);
        if(pthread_join(wts[i].thread, NULL) != 0) {
            printf("Error joining worker thread\n");
            exit(1);
        }
    }

    return wts;
}