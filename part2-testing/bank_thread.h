#ifndef BANK_THREAD_H
#define BANK_THREAD_H

#include "account.h"

typedef struct BankThreadHandlerArgs
{
    account *accounts;
    int num_accounts;
} BankThreadHandlerArgs;

void init_bank_thread(account *accounts, int num_accounts);

#endif