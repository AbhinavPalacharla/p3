#include "bank_thread.h"

void *bank_thread_handler(void *arg) {
    BankThreadHandlerArgs *args = (BankThreadHandlerArgs *) arg;

    issue_reward(args->accounts, args->num_accounts);

    return NULL;
}

void init_bank_thread(account *accounts, int num_accounts) {
    BankThreadHandlerArgs *args = (BankThreadHandlerArgs *) malloc(sizeof(BankThreadHandlerArgs));
    args->accounts = accounts;
    args->num_accounts = num_accounts;

    pthread_t bank_thread;

    pthread_create(&bank_thread, NULL, bank_thread_handler, (void *) args);

    if(pthread_join(bank_thread, NULL) != 0) {
        printf("Error joining bank thread\n");
        exit(1);
    }
}