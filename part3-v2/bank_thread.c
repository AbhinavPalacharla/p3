#include "bank_thread.h"
#include <pthread.h>
#include "utils.h"

extern pthread_mutex_t num_transactions_processed_mutex;
extern int num_transactions_processed;

extern int wakeup_bank_thread;
extern pthread_mutex_t wakeup_bank_thread_mutex;
extern pthread_cond_t wakeup_bank_thread_cond;

extern pthread_cond_t wakeup_worker_threads_cond;

void *bank_thread_handler(void *arg) {
    BankThreadHandlerArgs *args = (BankThreadHandlerArgs *) arg;

    while(true) {
        // while(wakeup_bank_thread == 0) {
        pthread_cond_wait(&wakeup_bank_thread_cond, &wakeup_bank_thread_mutex);
        // }

        printf("BANK THREAD RUNNING AFTER COND WAIT\n");

        issue_reward(args->accounts, args->num_accounts);

        view_accounts(args->accounts, args->num_accounts);

        pthread_mutex_lock(&num_transactions_processed_mutex);

        num_transactions_processed = 0;

        pthread_mutex_unlock(&num_transactions_processed_mutex);

        printf("WAKING UP WORKER THREADS\n");
        pthread_cond_broadcast(&wakeup_worker_threads_cond);

        printf("BANK THREAD FINISHED\n");
        return NULL;
    }
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