#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include "account.h"

enum TRNASACTION_TYPE
{
    DEPOSIT,
    WITHDRAW,
    TRANSFER,
    CHECK_BALANCE
};

typedef struct
{
    enum TRNASACTION_TYPE type;
    char account_number[17];
    char password[9];
    double amount;
    char destination_account[17];
} Transaction;

void view_transaction(Transaction *t);

Transaction *read_transaction(char *line);

void handle_transaction(Transaction *t, account *accounts, int num_accounts);

#endif