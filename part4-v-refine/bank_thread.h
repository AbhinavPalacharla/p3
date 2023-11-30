#ifndef BANK_THREAD_H
#define BANK_THREAD_H

#include "account.h"
#include <pthread.h>

typedef struct BankThreadHandlerArgs
{
    account *accounts;
    int num_accounts;
} BankThreadHandlerArgs;

typedef struct BankThread
{
    pthread_t thread;
    BankThreadHandlerArgs *args;
} BankThread;

BankThread *init_bank_thread(account *accounts, int num_accounts);

void join_bank_thread(BankThread *bt);

void free_bank_thread(BankThread *bt);

#endif