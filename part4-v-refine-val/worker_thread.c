#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include "worker_thread.h"
#include "utils.h"
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>

extern pthread_barrier_t barrier;
extern pthread_barrier_t barrier1;
extern pthread_barrier_t barrier2;
extern pthread_barrier_t bank_barrier;
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

extern int thread_exit_flag;

void *thread_handler(void *arg) {
    ThreadHandlerArgs *args = (ThreadHandlerArgs *) arg;
    int done = 0;
    int num_valid_trans = 0;
    int num_invalid_trans = 0;
    int num_total_trans = 0;
    // int done_counter = 0;

    printf("Thread %d Waiting...\n", args->id);
    pthread_barrier_wait(&barrier);
    printf("Thread %d Running...\n", args->id);
    while(true) {
        // int num_trans_unlock_done_flag = 0;
        pthread_mutex_lock(&num_transactions_processed_mutex);

        if(num_transactions_processed < REWARD_TRANSACTION_THRESHOLD && !done) {
            num_transactions_processed++;
            pthread_mutex_unlock(&num_transactions_processed_mutex);
            //keep dequing until you find valid transaction DONE
            //if hit null then mark as done and break out of loop DONE
            //also decrement num threads with work counter ONLY ONCE (protect with done) DONE

            /*
                lock num_trans_proc mutex while thread tries
                to find valid transaction and run it
                or it runs out of transactions
            */

                Transaction *t = args->tq->dequeue(args->tq);
                
                int ran_transaction = 0;

                while(ran_transaction == 0) {
                    //no transactions left
                    if(t == NULL) {
                        printf("T# %d RAN OUT OF TRANSACTIONS\n", args->id);

                        //mark thread as done in shared counter
                        if(!done) {
                            pthread_mutex_lock(&threads_running_mutex);
                                num_threads_with_work--;
                                printf("T# %d FINISHED | NUM THREADS WITH WORK %d\n", args->id, num_threads_with_work);
                            pthread_mutex_unlock(&threads_running_mutex);
                        }

                        done = 1; //mark yourself as done

                        //did not find anything so backtrack and decrement trans proc counter
                        if(!done) {
                            printf("############## T#%d IN NOT DONE ##############\n", args->id);
                            //pthread_mutex_lock(&num_transactions_processed_mutex);
                              //  num_transactions_processed--;
                            //pthread_mutex_unlock(&num_transactions_processed_mutex);
                        }
                        
                        break; //break out of loop then go sit at barrier in else
                    }
                    // printf("TRANS#: %d | T# %d RUNNING PROC | VALID#: %d, INVALID#: %d, TOTAL: %d\n", num_transactions_processed, args->id, num_valid_trans, num_invalid_trans, num_total_trans);

                    int ret = handle_transaction(t, args->accounts, args->num_accounts);
                    num_total_trans++;

                    if(ret != -1) {
                        num_valid_trans++;
                        ran_transaction = 1; //transaction successful break out of loop
                    } else {
                        // printf("T# %d RAN INVALID TRANS | VALID#: %d, INVALID#: %d, TOTAL: %d\n", args->id, num_valid_trans, num_invalid_trans, num_total_trans);
                        
                        num_invalid_trans++;

                        t = args->tq->dequeue(args->tq);
                    }
                    
                    // printf(">>TRANS#: %d | T# %d RUNNING PROC | VALID#: %d, INVALID#: %d, TOTAL: %d\n", num_transactions_processed, args->id, num_valid_trans, num_invalid_trans, num_total_trans);
                }
        } else {
        /*
            //unlock num_trans_proc mutex because we are done/threshold reached

            //if done make thread wait at some barrier
            //or if reached threshold also wait at some barrier (same barrier)

            //do another check of num transactions processed make sure it is 5k
            //if not 5k then let all threads try again

            
            if we reach 5k or nothing left (num_threads_with_work = 0)
            then thread0 sync with bank thread
            

           //if no threads left with work then exit
        */
            //release lock before waiting at barrier
        pthread_mutex_unlock(&num_transactions_processed_mutex); 

            if(done || num_transactions_processed >= REWARD_TRANSACTION_THRESHOLD) {
                // printf("T# %d WAITING AT BARRIER #1\n", args->id);
                pthread_barrier_wait(&barrier1);
                // printf("T# %d PASSED BARRIER #1\n", args->id);

                printf("T# %d | TOTAL COMPLETED %d, VALID: %d, INVALID: %d\n", args->id, num_total_trans, num_valid_trans, num_invalid_trans);

                //check again if 5k reached
                    int try_again = 0;

                    // if 5k not reached then sync with barrier and exit;
                    if(num_transactions_processed < REWARD_TRANSACTION_THRESHOLD) {
                        // printf("******************** CHECK AGAIN FOR 5K FAILED. NEED TO TRY AGAIN! ********************");
                        try_again = 1;
                    }

                    //???need barrier otherwise some other process will take mess with num_proc counter???
                    // printf("T# %d WAITING AT BARRIER #2\n", args->id);
                    pthread_barrier_wait(&barrier2);
                    // printf("T# %d PASSED BARRIER #2\n", args->id);
                    
                    // printf("T# %d TOTAL COMPLETED %d", args->id, num_total_trans);


                    //if counter is STILL 5k then signal bank thread OR all threads are done
                    if(!try_again || (num_threads_with_work == 0)) {
                        //signal bank thread that 5k complete and wait for it to signal back
                        if(args->id == 0) {
                            //signal bank to wakeup
                            pthread_mutex_lock(&wakeup_bank_thread_mutex);
                                printf("T# %d SIGNALING BANK\n", args->id);
                                pthread_cond_signal(&wakeup_bank_thread_cond);
                            pthread_mutex_unlock(&wakeup_bank_thread_mutex);

                            //reset num trans proc
                            // printf("RESETING NUM TRANSACTIONS PROCESSED\n");
                            num_transactions_processed = 0;
                            // printf("RESET NUM_TRANSACTIONS_PROCESSED = %d\n", num_transactions_processed);

                            //wait for bank to finish and wake t0 up
                            pthread_mutex_lock(&wakeup_worker_threads_mutex);
                                // printf("T# %d WAITING FOR WAKEUP\n", args->id);
                                pthread_cond_wait(&wakeup_worker_threads_cond, &wakeup_worker_threads_mutex);
                                printf("T# %d WOKEN UP BY BANK\n", args->id);
                            pthread_mutex_unlock(&wakeup_worker_threads_mutex);

                            if(num_threads_with_work == 0) {
                                //sync threads and exit
                                // printf("T# %d AT EXIT BARRIER\n", args->id);
                                // pthread_barrier_wait(&exit_barrier);
                                // printf("T# %d EXITING...\n", args->id);
                                // printf("T# %d SETTING THREAD EXIT FLAG\n", args->id);
                                thread_exit_flag = 1;
                                // exit(1);
                                //return NULL
                            }
                        }

                        //sync all threads
                        // printf("T# %d WAITING AT BANK BARRIER\n", args->id);
                        //t1-9 sit here until t0 hears back from bank then everything passes
                        pthread_barrier_wait(&bank_barrier);
                        // printf("T# %d PASSED BANK BARRIER\n", args->id);

                        if(thread_exit_flag == 1) {
                            // sleep(20);
                            printf("T# %d EXITING\n", args->id);
                            return NULL;
                        }

                        //check if all threads complete and exit
                        // if(num_threads_with_work == 0) {
                        //     //sync threads and exit
                        //     printf("T# %d AT EXIT BARRIER\n", args->id);
                        //     pthread_barrier_wait(&exit_barrier);
                        //     printf("T# %d EXITING...\n", args->id);
                        //     exit(1);
                        //     //return NULL
                        // }

                    }
            }
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

        for(int j = 0; j < num_thread_transactions; j++) {
            Transaction *t = tq->dequeue(tq);
            t->next = NULL;
            wts[i].tq->enqueue(wts[i].tq, t);
        }

        wts[i].args = (ThreadHandlerArgs *) malloc(sizeof(ThreadHandlerArgs));

        wts[i].args->id = i;
        wts[i].args->tq = wts[i].tq;
        wts[i].args->accounts = accounts;
        wts[i].args->num_accounts = num_accounts;
    }

    //create threads
    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&wts[i].thread, NULL, thread_handler, (void *) wts[i].args);
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

void free_worker_threads(WorkerThread *wts) {
    for(int i = 0; i < THREAD_POOL_SIZE; i++) {
        free_transactions_queue(wts[i].tq);
        free(wts[i].args);
    }

    free(wts);
}