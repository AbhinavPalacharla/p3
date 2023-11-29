#ifndef BANK_THREAD_H
#define BANK_THREAD_H

#include "account.h"

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

void free_bank_thread(BankThread *bt);

#endif