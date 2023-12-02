#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "transaction.h"
#include "account.h"

void free_transaction(Transaction *t) {
    free(t);
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
    
    //initialize transaction
    t->type = -1;
    strcpy(t->account_number, "");
    strcpy(t->password, "");
    t->amount = 0;
    strcpy(t->destination_account, "");

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
    }

    return t;
}


int handle_transaction(Transaction *t, account *accounts, int num_accounts) {
    int account_index = authenticate_account(t->account_number, t->password, accounts, num_accounts);

    if(account_index == -1) {
        // printf("ERROR: Invalid account number or password\n");
        return -1;
    }

    if(t->type == DEPOSIT) {
        accounts[account_index].balance += t->amount;
        accounts[account_index].transaction_tracter += t->amount;
    } else if(t->type == WITHDRAW) {
        accounts[account_index].balance -= t->amount;
        accounts[account_index].transaction_tracter += t->amount;
    } else if(t->type == TRANSFER) {
        // return;
        int dest = find_account(t->destination_account, accounts, num_accounts);

        if(dest == -1) {
            // printf("ERROR: Invalid destination account number\n");
            return -1;
        }

        accounts[account_index].balance -= t->amount;
        accounts[account_index].transaction_tracter += t->amount;
        accounts[dest].balance += t->amount;
    } else if(t->type == CHECK_BALANCE) {
        // printf("BALANCE: %f\n", accounts[account_index].balance);
        return 1;
    }

    return 0;
}