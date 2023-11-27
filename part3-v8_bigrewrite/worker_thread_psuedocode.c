#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include "worker_thread.h"
#include "utils.h"
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>

typedef struct _ThreadHandlerArgs {
    int id;
    TransactionQueue *tq;
    account *accounts;
    int num_accounts;
} ThreadHandlerArgs;

extern pthread_barrier_t barrier;
extern pthread_barrier_t exit_barrier;

//transaction processing sync vars
extern int num_transactions_processed;
extern pthread_mutex_t num_transactions_processed_mutex;
extern pthread_cond_t num_transactions_processed_cond;

//wakeup bank thread sync vars
extern pthread_mutex_t wakeup_bank_thread_mutex;
extern pthread_cond_t wakeup_bank_thread_cond;

//wakeup worker threads sync vars
extern pthread_mutex_t wakeup_worker_threads_mutex;
extern pthread_cond_t wakeup_worker_threads_cond;

//num threads running
extern pthread_mutex_t threads_running_mutex;
extern int num_threads_with_work;

extern pthread_mutex_t threads_waiting_for_bcast_mutex;
extern int num_threads_waiting_for_bcast;

void *thread_handler(void *arg) {
    ThreadHandlerArgs *args = (ThreadHandlerArgs *) arg;
    int done = 0;
    int num_valid_trans = 0;
    int num_invalid_trans = 0;
    int num_total_trans = 0;
    int done_counter = 0;

    printf("Thread %d Waiting...\n", args->id);
    pthread_barrier_wait(&barrier);
    printf("Thread %d Running...\n", args->id);
    while(true) {
        // int num_trans_unlock_done_flag = 0;
        pthread_mutex_lock(&num_transactions_processed_mutex);

        if(num_transactions_processed < REWARD_TRANSACTION_THRESHOLD && !done) {
            num_transactions_processed++;
            pthread_mutex_unlock(&num_transactions_processed_mutex);

            //keep dequing until you find valid transaction
            //if hit null then mark as done and break out of loop
            //also decrement num threads with work counter ONLY ONCE (protect with done)

        } else {
            //unlock num_trans_proc mutex because we are done/threshold reached

            //if done make thread wait at some barrier
            //or if reached threshold also wait at some barrier (same barrier)

            //do another check of num transactions processed make sure it is 5k
            //if not 5k then let all threads try again

            /*
            if we reach 5k or nothing left (num_threads_with_work = 0)
            then thread0 sync with bank thread
            */

           //if no threads left with work then exit
        }        
         
    }
}

WorkerThread *init_worker_threads(TransactionQueue *tq, account *accounts, int num_accounts) {
    WorkerThread *wts = (WorkerThread *) malloc(sizeof(WorkerThread) * THREAD_POOL_SIZE);

    int num_thread_transactions = tq->size / THREAD_POOL_SIZE;
    // int num_thread_transactions = 100 / THREAD_POOL_SIZE;


    printf("NUM THREAD TRANSACTIONS: %d\n", num_thread_transactions);

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        wts[i].tq = init_transaction_queue();
    }

    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        for(int j = 0; j < num_thread_transactions; j++) {
            Transaction *t = tq->dequeue(tq);
            t->next = NULL;
            wts[i].tq->enqueue(wts[i].tq, t);
        }
        printf("Size of T# %d: %d\n", i, wts[i].tq->size);
    }

    //create threads
    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        ThreadHandlerArgs *args = (ThreadHandlerArgs *) malloc(sizeof(ThreadHandlerArgs));
        
        args->id = i;
        args->tq = wts[i].tq;
        args->accounts = accounts;
        args->num_accounts = num_accounts;

        pthread_create(&wts[i].thread, NULL, thread_handler, (void *) args);
    }

    return wts;
}

void join_worker_threads(WorkerThread *wts) {
    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        if(pthread_join(wts[i].thread, NULL) != 0) {
            printf("Error joining worker thread\n");
            exit(1);
        }
    }
}