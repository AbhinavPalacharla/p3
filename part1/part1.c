#include <stdio.h>
#include <stdlib.h>
#include "account.h"
#include <string.h>

enum TRNASACTION_TYPE {
    DEPOSIT,
    WITHDRAW,
    TRANSFER,
    CHECK_BALANCE
};

typedef struct {
    enum TRNASACTION_TYPE type;
    char account_number[17];
    char password[9];
    double amount;
    char destination_account[17];
} Transaction;

char *strip(char *str) {
    if(str[strlen(str) - 1] == '\n') {
        str[strlen(str) - 1] = '\0';
    }

    return str;
}

void read_accounts(account *accounts, FILE *f, int num_accounts) {
    size_t len = 128; char *line = malloc(sizeof(char) * len);  ssize_t read;

    for(int i = 0; i < num_accounts; i++) {
        getline(&line, &len, f); //skip index line

        read = getline(&line, &len, f); // account number
        strcpy(accounts[i].account_number, strip(line));

        read = getline(&line, &len, f); // account password
        strcpy(accounts[i].password, strip(line));

        read = getline(&line, &len, f); // initial balance
        accounts[i].balance = atof(strip(line));

        read = getline(&line, &len, f); // reward rate
        accounts[i].reward_rate = strtod(strip(line), NULL);

        accounts[i].transaction_tracter = 0;
    }

    free(line);
}

void view_accounts(account *accounts, int num_accounts) {
    printf("ACCOUNTS:\n\n");

    for(int i = 0; i < num_accounts; i++) {
        printf("[%d/%d]\t (#) %s\t (PASSWD) %s\t (BAL) %f\t RR %lf \n", i + 1, num_accounts, accounts[i].account_number, accounts[i].password, accounts[i].balance, accounts[i].reward_rate);
    }

    printf("\n");
}

void free_accounts(account *accounts, int num_accounts) {
    return;
}

int authenticate_account(char *account_number, char *password, account *accounts, int num_accounts) {
    for(int i = 0; i < num_accounts; i++) {
        if(strcmp(account_number, accounts[i].account_number) == 0) {
            if(strcmp(password, accounts[i].password) == 0) {
                return i;
            }
        }
    }

    printf("Failed to authenticate account %s | %s\n", account_number, password);

    return -1;
}

int find_account(char *account_number, account *accounts, int num_accounts) {
    for(int i = 0; i < num_accounts; i++) {
        if(strcmp(account_number, accounts[i].account_number) == 0) {
            return i;
        }
    }

    return -1;
}

void view_transaction(Transaction *t) {
    printf("TRANSACTION | ");

    switch(t->type) {
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
    if(t->type == TRANSFER) { printf("(DEST) %s\t", t->destination_account); }
    printf("(AMOUNT) %f\n", t->amount);
}

Transaction *get_transaction(char *line) {
    Transaction *t = (Transaction *) malloc(sizeof(Transaction));

    //get transaction type
    char *token = strip(strtok(line, " "));
    // printf("Type: %s\n", token);
    if(strcmp(token, "D") == 0) {
        t->type = DEPOSIT;
    } else if(strcmp(token, "W") == 0) {
        t->type = WITHDRAW;
    } else if(strcmp(token, "T") == 0) {
        t->type = TRANSFER;
    } else if (strcmp(token, "C") == 0) {
        t->type = CHECK_BALANCE;
    }

    //get account number
    token = strtok(NULL, " ");
    strcpy(t->account_number, strip(token));
    // printf("Account number: %s\n", t->account_number);

    //get password
    token = strtok(NULL, " ");
    strcpy(t->password, strip(token));
    // printf("Password: %s\n", t->password);

    if(t->type == TRANSFER) {
        //get destination account
        token = strtok(NULL, " ");
        strcpy(t->destination_account, strip(token));
    } else {
        strcpy(t->destination_account, "");
        // printf("Destination account: %s\n", t->destination_account);
    }

    //get amount
    if(t->type != CHECK_BALANCE) {
        token = strtok(NULL, " ");
        t->amount = atof(strip(token));
        // printf("Amount: %f\n", t->amount);
    }
    
    return t;
}

void handle_transaction(Transaction *t, account *accounts, int num_accounts) {
    int account_index = authenticate_account(t->account_number, t->password, accounts, num_accounts);

    if(account_index == -1) {
        printf("ERROR: Invalid account number or password\n");
        return;
    }

    if(t->type == DEPOSIT) {
        accounts[account_index].balance += t->amount;
        accounts[account_index].transaction_tracter += t->amount;
    } else if(t->type == WITHDRAW) {
        accounts[account_index].balance -= t->amount;
        accounts[account_index].transaction_tracter += t->amount;
    } else if(t->type == TRANSFER) {
        int dest = find_account(t->destination_account, accounts, num_accounts);

        if(dest == -1) {
            printf("ERROR: Invalid destination account number\n");
            return;
        }

        accounts[account_index].balance -= t->amount;
        accounts[account_index].transaction_tracter += t->amount;
        accounts[dest].balance += t->amount;
    } else if(t->type == CHECK_BALANCE) {
        printf("BALANCE: %f\n", accounts[account_index].balance);
    }
}

void issue_reward(account *accounts, int num_accounts) {
    for(int i = 0; i < num_accounts; i++) {
        accounts[i].balance += accounts[i].transaction_tracter * accounts[i].reward_rate;
    }
}

int main(int argc, char **argv) {
    FILE *f;
    
    if((f = fopen(argv[1], "r")) == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    int num_accounts;
    size_t len = 128; char *line = malloc(sizeof(char) * len);  ssize_t read;

    read = getline(&line, &len, f);

    num_accounts = atoi(line);

    printf("Number of accounts: %d\n", num_accounts);

    account *accounts = (account *) malloc(sizeof(account) * num_accounts); //array of accounts

    read_accounts(accounts, f, num_accounts); //populate accounts

    view_accounts(accounts, num_accounts);

    //read transactions
    while((read = getline(&line, &len, f)) != -1) {
        Transaction *t = get_transaction(line);
        view_transaction(t);
        handle_transaction(t, accounts, num_accounts);
    }

    issue_reward(accounts, num_accounts);

    printf("\n");

    view_accounts(accounts, num_accounts);

    free(line);
    free_accounts(accounts, num_accounts);

    return 0;
}