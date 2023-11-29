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
        int num_trans_unlock_done_flag = 0;
        pthread_mutex_lock(&num_transactions_processed_mutex);
            // if((num_transactions_processed >= REWARD_TRANSACTION_THRESHOLD)) {
            if((num_transactions_processed >= REWARD_TRANSACTION_THRESHOLD) || done) {
                num_trans_unlock_done_flag = 1;
                pthread_mutex_unlock(&num_transactions_processed_mutex);
                printf("T# %d WAITING AT BARRIER\n", args->id);
                pthread_barrier_wait(&barrier); //wait for all threads to finish their handling
                printf("T# %d PASSED BARRIER\n", args->id);

                //signal bank thread to wakeup
                if(args->id == 0) {
                    pthread_mutex_lock(&wakeup_bank_thread_mutex);
                        printf("T# %d SIGNALING BANK\n", args->id);
                        pthread_cond_signal(&wakeup_bank_thread_cond);

                    pthread_mutex_unlock(&wakeup_bank_thread_mutex);

                    printf("RESETING NUM TRANSACTIONS PROCESSED\n");
                    pthread_mutex_lock(&num_transactions_processed_mutex);

                        num_transactions_processed = 0;
                        printf("RESET NUM_TRANSACTIONS_PROCESSED = %d\n", num_transactions_processed);

                    pthread_mutex_unlock(&num_transactions_processed_mutex);            

                    //wait for bank thread to finish and listen for wakeup signal
                    pthread_mutex_lock(&wakeup_worker_threads_mutex);

                        printf("T# %d WAITING FOR WAKEUP\n", args->id);

                        pthread_cond_wait(&wakeup_worker_threads_cond, &wakeup_worker_threads_mutex);

                        printf("T# %d WOKEN UP BY BANK\n", args->id);

                    pthread_mutex_unlock(&wakeup_worker_threads_mutex);

                }

                printf("T# %d WAITING AT BANK BARRIER\n", args->id);

                pthread_barrier_wait(&barrier); //wait for all threads to finish their handling

                printf("T# %d PASSED BANK BARRIER\n", args->id);

                if(num_threads_with_work == 0) {
                    // view_accounts(args->accounts, args->num_accounts);
                    printf("T# %d EXITING | CT %d\n", args->id, done_counter);
                    done_counter++;
                    // exit(1);
                    printf("T# %d WAITING BEFORE EXIT BARRIER\n", args->id);
                    pthread_barrier_wait(&exit_barrier);
                    printf("T# %d WAKEUP FROM EXIT BARRIER\n", args->id);
                    // pthread_exit(0);
                    // return NULL;
                    // return NULL;
                }
            }
        
            if(!done) {
            //pthread_mutex_lock(&num_transactions_processed_mutex);

                num_transactions_processed++;

                num_valid_trans += 1;
                num_total_trans += 1;
            }
         
        if (!num_trans_unlock_done_flag) { pthread_mutex_unlock(&num_transactions_processed_mutex); }

            printf("TRANS#: %d | T# %d RUNNING PROC | VALID#: %d, INVALID#: %d, TOTAL: %d\n", num_transactions_processed, args->id, num_valid_trans, num_invalid_trans, num_total_trans);

            // printf("TRANS#: %d | T# %d RUNNING PROC | TOTAL: %d\n", num_transactions_processed, args->id, num_total_trans);

        // if(!num_trans_unlock_done_flag) { pthread_mutex_unlock(&num_transactions_processed_mutex); }

        Transaction *t = args->tq->dequeue(args->tq);

        if(t == NULL) { 
            if(!done) {
                printf("T# %d FINISHED\n", args->id); 
                done = 1;
                
                pthread_mutex_lock(&threads_running_mutex);
                    num_threads_with_work--;
                pthread_mutex_unlock(&threads_running_mutex);
                
                printf("NUM THREADS WORKING: %d\n", num_threads_with_work);
                
                if(num_threads_with_work == 0) {
                    printf("T# %d ALL THREADS DONE\n", args->id);
                }
            }
            
            // while(1);

                // if(!done) {
                    
                //     printf("T# %d FINISHED\n", args->id); 

                //     pthread_mutex_lock(&threads_running_mutex);

                //         num_threads_with_work--;

                //     pthread_mutex_unlock(&threads_running_mutex);

                //     done = 1;
                // }

        } else {
            if(handle_transaction(t, args->accounts, args->num_accounts) == -1) {
                pthread_mutex_lock(&num_transactions_processed_mutex);
                
                    num_transactions_processed--;
                    num_valid_trans -= 1;
                    num_invalid_trans += 1;
                
                pthread_mutex_unlock(&num_transactions_processed_mutex);
            }
            
            // sched_yield();
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