#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "transaction.h"
#include "account.h"
#include <time.h>

void enqueue(TransactionQueue *self, Transaction *t)
{
    if (self->size == 0) {
        self->head = t;
        self->tail = t;
    } else {
        self->tail->next = t;
        self->tail = t;
    }

    self->size++;
}

Transaction *dequeue(TransactionQueue *self)
{
    if (self->size == 0) { return NULL; }

    Transaction *t = self->head;
    self->head = self->head->next;
    self->size--;

    return t;
}

TransactionQueue *init_transaction_queue()
{
    TransactionQueue *q = (TransactionQueue *)malloc(sizeof(TransactionQueue));
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    
    q->enqueue = &enqueue;
    q->dequeue = &dequeue;
    
    return q;
}

void view_transaction_queue(TransactionQueue *tq)
{
    Transaction *t = tq->head;

    while (t != NULL)
    {
        view_transaction(t);
        t = t->next;
    }

    printf("\n");
}

void free_transaction(Transaction *t)
{
    free(t);
}

void free_transactions_queue(TransactionQueue *tq)
{
    Transaction *t = tq->head;
    Transaction *next;

    while (t != NULL)
    {
        next = t->next;
        free_transaction(t);
        t = next;
    }

    free(tq);
}

void view_transaction(Transaction *t)
{
    printf("TRANSACTION | ");

    switch (t->type)
    {
    case DEPOSIT:
        printf("DEPOSIT\t");
        break;
    case WITHDRAW:
        printf("WITHDRAW\t");
        break;
    case TRANSFER:
        printf("TRANSFER\t");
        break;
    case CHECK_BALANCE:
        printf("CHECK_BALANCE\t");
        break;
    }

    printf("(#) %s\t", t->account_number);
    printf("(PASSWD) %s\t", t->password);
    if (t->type == TRANSFER)
    {
        printf("(DEST) %s\t", t->destination_account);
    }
    printf("(AMOUNT) %f\n", t->amount);
}

Transaction *read_transaction(char *line)
{
    Transaction *t = (Transaction *)malloc(sizeof(Transaction));

    // get transaction type
    char *token = strip(strtok(line, " "));
    // printf("Type: %s\n", token);
    if (strcmp(token, "D") == 0)
    {
        t->type = DEPOSIT;
    }
    else if (strcmp(token, "W") == 0)
    {
        t->type = WITHDRAW;
    }
    else if (strcmp(token, "T") == 0)
    {
        t->type = TRANSFER;
    }
    else if (strcmp(token, "C") == 0)
    {
        t->type = CHECK_BALANCE;
    }

    // get account number
    token = strtok(NULL, " ");
    strcpy(t->account_number, strip(token));
    // printf("Account number: %s\n", t->account_number);

    // get password
    token = strtok(NULL, " ");
    strcpy(t->password, strip(token));
    // printf("Password: %s\n", t->password);

    if (t->type == TRANSFER)
    {
        // get destination account
        token = strtok(NULL, " ");
        strcpy(t->destination_account, strip(token));
    }
    else
    {
        strcpy(t->destination_account, "");
        // printf("Destination account: %s\n", t->destination_account);
    }

    // get amount
    if (t->type != CHECK_BALANCE)
    {
        token = strtok(NULL, " ");
        t->amount = atof(strip(token));
        // printf("Amount: %f\n", t->amount);
    }

    return t;
}


int handle_transaction(Transaction *t, account *accounts, int num_accounts) {
    // struct timespec ts;
    // ts.tv_sec = 0;
    // ts.tv_nsec = 100000;


    // nanosleep(&ts, NULL);
    
    int account_index = authenticate_account(t->account_number, t->password, accounts, num_accounts);

    if(account_index == -1) {
        printf("ERROR: Failed to Authenicate. Invalid account number or password\n");
        return -1;
    }

    if(t->type == DEPOSIT) {
        pthread_mutex_lock(&accounts[account_index].ac_lock);
        
        accounts[account_index].balance += t->amount;
        accounts[account_index].transaction_tracter += t->amount;
        
        pthread_mutex_unlock(&accounts[account_index].ac_lock);
    } else if(t->type == WITHDRAW) {
        pthread_mutex_lock(&accounts[account_index].ac_lock);

        accounts[account_index].balance -= t->amount;
        accounts[account_index].transaction_tracter += t->amount;

        pthread_mutex_unlock(&accounts[account_index].ac_lock);
    } else if(t->type == TRANSFER) {
        // return 0;

        int dest = find_account(t->destination_account, accounts, num_accounts);

        if(dest == -1) {
            printf("ERROR: Invalid destination account number\n");
            return -1;
        }

        pthread_mutex_lock(&accounts[account_index].ac_lock);
        
        accounts[account_index].balance -= t->amount;
        accounts[account_index].transaction_tracter += t->amount;
        
        pthread_mutex_unlock(&accounts[account_index].ac_lock);

        pthread_mutex_lock(&accounts[dest].ac_lock);

        
        accounts[dest].balance += t->amount;

        pthread_mutex_unlock(&accounts[dest].ac_lock);
    } else if(t->type == CHECK_BALANCE) {
        // pthread_mutex_lock(&accounts[account_index].ac_lock);

        // printf("BALANCE: %f\n", accounts[account_index].balance);

        // pthread_mutex_unlock(&accounts[account_index].ac_lock);

        return -1;
    }

    return 0;
}