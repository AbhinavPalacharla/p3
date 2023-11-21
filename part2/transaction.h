#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include "account.h"
#include <pthread.h>

enum TRNASACTION_TYPE
{
    DEPOSIT,
    WITHDRAW,
    TRANSFER,
    CHECK_BALANCE
};

typedef struct _Transaction
{
    enum TRNASACTION_TYPE type;
    char account_number[17];
    char password[9];
    double amount;
    char destination_account[17];
    struct Transaction *next;
} Transaction;

typedef struct _TransactionsQueue 
{
    Transaction *head;
    Transaction *tail;
    int size;
    pthread_mutex_t lock;
    
    void (*enqueue)(struct TransactionsQueue *self, Transaction *t);
    Transaction *(*dequeue)(struct TransactionsQueue *self); 
} TransactionsQueue;

TransactionsQueue *init_transactions_queue();
void enqueue(TransactionsQueue *self, Transaction *t);
Transaction *dequeue(TransactionsQueue *self);

void view_transaction(Transaction *t);

Transaction *get_transaction(char *line);

void handle_transaction(Transaction *t, account *accounts, int num_accounts);

void free_transactions_queue(TransactionsQueue *tq);

void free_transaction(Transaction *t);

#endif