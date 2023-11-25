#ifndef BANK_THREAD_H
#define BANK_THREAD_H

#include "account.h"
#include <pthread.h>

typedef struct BankThreadHandlerArgs
{
    account *accounts;
    int num_accounts;
} BankThreadHandlerArgs;

pthread_t init_bank_thread(account *accounts, int num_accounts);
void join_bank_thread(pthread_t bank_thread);

#endif