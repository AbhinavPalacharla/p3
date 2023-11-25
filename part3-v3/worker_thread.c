#define _XOPEN_SOURCE 600
#include "worker_thread.h"
#include "utils.h"
#include <pthread.h>

extern int num_threads_with_work;
extern pthread_mutex_t threads_running_mutex;

extern pthread_barrier_t barrier;
extern int num_transactions_processed;
extern pthread_mutex_t num_transactions_processed_mutex;
extern pthread_cond_t num_transactions_processed_cond;

extern int wakeup_bank_thread;
// extern pthread_mutex_t wakeup_bank_thread_mutex;
extern pthread_cond_t wakeup_bank_thread_cond;

extern int wakeup_worker_threads;
extern pthread_mutex_t wakeup_worker_threads_mutex;
extern pthread_cond_t wakeup_worker_threads_cond;

typedef struct _ThreadHandlerArgs {
    int id;
    TransactionQueue *tq;
    account *accounts;
    int num_accounts;
} ThreadHandlerArgs;

void *thread_handler(void *arg) {
    ThreadHandlerArgs *args = (ThreadHandlerArgs *) arg;

    printf("Thread %d Waiting...\n", args->id);
    pthread_barrier_wait(&barrier);
    printf("Thread %d Running...\n", args->id);

    while(true) {
        printf("T# %d WAITING FOR NUM_TRANS LOCK\n", args->id);
        pthread_mutex_lock(&num_transactions_processed_mutex);
            //if transactions processed is >= threshold, wait
            if(num_transactions_processed >= REWARD_TRANSACTION_THRESHOLD) {

               printf("T# %d AT THRESHOLD BARRIER\n", args->id);
               printf("NUM TRANSACTIONS PROCESSED: %d\n", num_transactions_processed);
               pthread_mutex_unlock(&num_transactions_processed_mutex);
               pthread_barrier_wait(&barrier); //wait for all threads to be ready again

               printf("NUM TRANSACTIONS PROCESSED: %d\n", num_transactions_processed);
               printf("T# %d PASSED THRESHOLD BARRIER\n", args->id);

               //start bank thread
                if(args->id == 0) {
                    printf("Restarting bank thread...\n");
                    wakeup_bank_thread = 1;
                    pthread_cond_signal(&wakeup_bank_thread_cond);
                }
                // while(1);

                // //wait for bank thread to finish
            
                printf("T# %d BEFORE COND WAIT\n", args->id);

                // while(wakeup_worker_threads == 0) {
                pthread_cond_wait(&wakeup_worker_threads_cond, &wakeup_worker_threads_mutex);
                // }

                printf("T# %d WOKE UP BEFORE BARRIER\n", args->id);

                // pthread_barrier_wait(&barrier);

                // printf("T# %d WOKE UP AFTER BARRIER\n", args->id);
                
                // if(args->id == 0) {
                //     wakeup_worker_threads = 0;
                // }
                // wakeup_worker_threads = 0;

                pthread_mutex_unlock(&wakeup_worker_threads_mutex);
            }

            //increment num transactions processed
            num_transactions_processed++;

            printf("NUM TRANS PROC: %d | T# %d UNLOCKED\n", num_transactions_processed, args->id);

        pthread_mutex_unlock(&num_transactions_processed_mutex);

        // printf("T#%d PROCESSING TRANSACTION\n", args->id);
        //process transaction
        Transaction *t = args->tq->dequeue(args->tq);

        //NOTE EXIT EARLY WILL MESS WITH BARRIER. FIX THIS LATER
        if(t == NULL) { 
            printf("T# %d FINISHED", args->id); 

            pthread_mutex_lock(&threads_running_mutex);
            num_threads_with_work--;
            printf("THREADS RUNNING: %d\n", num_threads_with_work);
            pthread_mutex_unlock(&threads_running_mutex);    
        }

        //Only runs if there is a transaction for thread to process
        if(t != NULL && (handle_transaction(t, args->accounts, args->num_accounts) == -1)) {
            //transaciton invalid - decrement num transactions processed
            pthread_mutex_lock(&num_transactions_processed_mutex);
            
                num_transactions_processed--;
            
            pthread_mutex_unlock(&num_transactions_processed_mutex);
        }

        printf("T# %d TRANSACTION PROCESSED\n", args->id);
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