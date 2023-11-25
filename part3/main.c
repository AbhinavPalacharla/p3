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

pthread_barrier_t barrier;
int num_transactions_processed = 0;
pthread_mutex_t num_transactions_processed_mutex;
pthread_cond_t num_transactions_processed_cond;

int wakeup_bank_thread = 0;
pthread_mutex_t wakeup_bank_thread_mutex;
pthread_cond_t wakeup_bank_thread_cond;

int wakeup_worker_threads = 0;
pthread_mutex_t wakeup_worker_threads_mutex;
pthread_cond_t wakeup_worker_threads_cond;

int main(int argc, char **argv) {
    pthread_barrier_init(&barrier, NULL, THREAD_POOL_SIZE);
    pthread_mutex_init(&num_transactions_processed_mutex, NULL);
    pthread_cond_init(&num_transactions_processed_cond, NULL);

    pthread_mutex_init(&wakeup_bank_thread_mutex, NULL);
    // pthread_mutex_lock(&wakeup_bank_thread_mutex);
    pthread_cond_init(&wakeup_bank_thread_cond, NULL);

    pthread_mutex_init(&wakeup_worker_threads_mutex, NULL);
    // pthread_mutex_lock(&wakeup_worker_threads_mutex);
    pthread_cond_init(&wakeup_worker_threads_cond, NULL);


    FILE *f;
    if((f = fopen(argv[1], "r")) == NULL) { printf("Error opening file\n"); exit(1); }

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

    /*BANK THREAD*/
    pthread_t bank_thread = init_bank_thread(accounts, num_accounts);

    /*WORKER THREADS*/
    WorkerThread *wts = init_worker_threads(tq, accounts, num_accounts);

    join_worker_threads(wts);
    join_bank_thread(bank_thread);

    // /*BANK THREAD*/
    // init_bank_thread(accounts, num_accounts);

    /*THE END*/
    printf("\n");

    view_accounts(accounts, num_accounts);

    free_transactions_queue(tq);

    fclose(f);

    return 0;
}
