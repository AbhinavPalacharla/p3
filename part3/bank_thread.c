#include "bank_thread.h"
#include <pthread.h>
#include "utils.h"
#include <time.h>

extern int num_transactions_processed;

//bank thread sync vars
extern pthread_mutex_t wakeup_bank_thread_mutex;
extern pthread_cond_t wakeup_bank_thread_cond;

//worker thread sync vars
extern pthread_mutex_t wakeup_worker_threads_mutex;
extern pthread_cond_t wakeup_worker_threads_cond;

extern int num_threads_with_work;
extern pthread_mutex_t threads_running_mutex;

extern int num_threads_with_work;

void *bank_thread_handler(void *arg) {
    BankThreadHandlerArgs *args = (BankThreadHandlerArgs *) arg;

    while(true) {
        //listen for wakeup
        pthread_mutex_lock(&wakeup_bank_thread_mutex);

            pthread_cond_wait(&wakeup_bank_thread_cond, &wakeup_bank_thread_mutex);

            issue_reward(args->accounts, args->num_accounts);

            view_accounts(args->accounts, args->num_accounts);

            num_transactions_processed = 0;

        pthread_mutex_unlock(&wakeup_bank_thread_mutex);

        //wake up worker threads
        pthread_mutex_lock(&wakeup_worker_threads_mutex);

            pthread_cond_broadcast(&wakeup_worker_threads_cond);

        pthread_mutex_unlock(&wakeup_worker_threads_mutex);

        if(num_threads_with_work == 0) {
            return NULL;
        }
    }

    return NULL;
}

pthread_t init_bank_thread(account *accounts, int num_accounts) {
    BankThreadHandlerArgs *args = (BankThreadHandlerArgs *) malloc(sizeof(BankThreadHandlerArgs));
    args->accounts = accounts;
    args->num_accounts = num_accounts;

    pthread_t bank_thread;

    pthread_create(&bank_thread, NULL, bank_thread_handler, (void *) args);

    return bank_thread;

    // if(pthread_join(bank_thread, NULL) != 0) {
    //     printf("Error joining bank thread\n");
    //     exit(1);
    // }
}

void join_bank_thread(pthread_t bank_thread) {
    if(pthread_join(bank_thread, NULL) != 0) {
        printf("Error joining bank thread\n");
        exit(1);
    }
}