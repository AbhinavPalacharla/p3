#define _XOPEN_SOURCE 600
#include "worker_thread.h"
#include "utils.h"
#include <pthread.h>

typedef struct _ThreadHandlerArgs {
    int id;
    TransactionQueue *tq;
    account *accounts;
    int num_accounts;
} ThreadHandlerArgs;

extern pthread_barrier_t barrier;

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

void *thread_handler(void *arg) {
    ThreadHandlerArgs *args = (ThreadHandlerArgs *) arg;
    int done = 0;
    int num_valid_trans = 0;
    int num_invalid_trans = 0;
    int num_total_trans = 0;

    printf("Thread %d Waiting...\n", args->id);
    pthread_barrier_wait(&barrier);
    printf("Thread %d Running...\n", args->id);


    while(true) {        
        pthread_mutex_unlock(&num_transactions_processed_mutex);

            if((num_transactions_processed >= REWARD_TRANSACTION_THRESHOLD) || done) {
                pthread_barrier_wait(&barrier); //wait for all threads to finish their handling

                //signal bank thread to wakeup
                pthread_mutex_lock(&wakeup_bank_thread_mutex);

                    pthread_cond_signal(&wakeup_bank_thread_cond);

                pthread_mutex_unlock(&wakeup_bank_thread_mutex);

                //wait for bank thread to finish and listen for wakeup signal
                pthread_mutex_lock(&wakeup_worker_threads_mutex);

                    pthread_cond_wait(&wakeup_worker_threads_cond, &wakeup_worker_threads_mutex);

                    if(num_threads_with_work == 0) {
                        return NULL;
                    }

                pthread_mutex_unlock(&wakeup_worker_threads_mutex);
            }
        
            if(!done) {
                num_transactions_processed++;
                num_valid_trans += 1;
                num_total_trans += 1;
            }
            // num_transactions_processed++;
            // num_valid_trans += 1;
            // num_total_trans += 1;

            printf("TRANS#: %d | T# %d RUNNING PROC | VALID#: %d, INVALID#: %d, TOTAL: %d\n", num_transactions_processed, args->id, num_valid_trans, num_invalid_trans, num_total_trans);

        pthread_mutex_unlock(&num_transactions_processed_mutex);

        Transaction *t = args->tq->dequeue(args->tq);

        if(t == NULL) { 
            // printf("T# %d FINISHED\n", args->id); 

                if(!done) {
                    
                    printf("T# %d FINISHED\n", args->id); 

                    pthread_mutex_lock(&threads_running_mutex);

                        num_threads_with_work--;

                    pthread_mutex_unlock(&threads_running_mutex);

                    done = 1;
                }

        } else if(handle_transaction(t, args->accounts, args->num_accounts) == -1) {
            pthread_mutex_lock(&num_transactions_processed_mutex);
            
                num_transactions_processed--;
                num_valid_trans -= 1;
                num_invalid_trans += 1;
            
            pthread_mutex_unlock(&num_transactions_processed_mutex);
        }
    }

    return NULL;
}

WorkerThread *init_worker_threads(TransactionQueue *tq, account *accounts, int num_accounts) {
    WorkerThread *wts = (WorkerThread *) malloc(sizeof(WorkerThread) * THREAD_POOL_SIZE);

    int num_thread_transactions = tq->size / THREAD_POOL_SIZE;

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