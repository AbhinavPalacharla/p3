#include "bank_thread.h"
#include <pthread.h>
#include "utils.h"
#include <time.h>
#include <unistd.h>

extern int num_transactions_processed;
extern pthread_mutex_t num_transactions_processed_mutex;

//bank thread sync vars
extern pthread_mutex_t wakeup_bank_thread_mutex;
extern pthread_cond_t wakeup_bank_thread_cond;

//worker thread sync vars
extern pthread_mutex_t wakeup_worker_threads_mutex;
extern pthread_cond_t wakeup_worker_threads_cond;

extern int num_threads_with_work;
extern pthread_mutex_t threads_running_mutex;

extern int num_threads_with_work;

extern pthread_mutex_t threads_waiting_for_bcast_mutex;
extern int num_threads_waiting_for_bcast;

void *bank_thread_handler(void *arg) {
    BankThreadHandlerArgs *args = (BankThreadHandlerArgs *) arg;

    while(true) {
        //listen for wakeup
        pthread_mutex_lock(&wakeup_bank_thread_mutex);

            // printf("BANK THREAD WAITING FOR WAKEUP\n");
            pthread_cond_wait(&wakeup_bank_thread_cond, &wakeup_bank_thread_mutex);
            // printf("BANK THREAD WOKEN UP\n");

        pthread_mutex_unlock(&wakeup_bank_thread_mutex);
        

        // printf("BANK THREAD ISSUING REWARD\n");
        issue_reward(args->accounts, args->num_accounts);
        // printf("BANK THREAD ISSUED REWARDS\n");
        
        view_accounts(args->accounts, args->num_accounts);

        //wake up worker threads
        pthread_mutex_lock(&wakeup_worker_threads_mutex);

            // printf("BANK THREAD WAKING UP WORKER THREAD 0\n");

            pthread_cond_signal(&wakeup_worker_threads_cond);

        pthread_mutex_unlock(&wakeup_worker_threads_mutex);

        if(num_threads_with_work == 0) {
            printf("NO WORKER THREADS RUNNING, EXITING BANK THREAD\n");
            while(1);
            // return NULL;
        }
    }
}

BankThread *init_bank_thread(account *accounts, int num_accounts) {
    // BankThreadHandlerArgs *args = (BankThreadHandlerArgs *) malloc(sizeof(BankThreadHandlerArgs));
    // args->accounts = accounts;
    // args->num_accounts = num_accounts;

    // pthread_t bank_thread;

    BankThread *bt = (BankThread *) malloc(sizeof(BankThread));

    bt->args = (BankThreadHandlerArgs *) malloc(sizeof(BankThreadHandlerArgs));
    bt->args->accounts = accounts;
    bt->args->num_accounts = num_accounts;

    pthread_create(&bt->thread, NULL, bank_thread_handler, (void *) bt->args);

    return bt;
}

void join_bank_thread(BankThread *bt) {
    if(pthread_join(bt->thread, NULL) != 0) {
        printf("Error joining bank thread\n");
        exit(1);
    }
}

void free_bank_thread(BankThread *bt) {
    free(bt->args);
    free(bt);
}