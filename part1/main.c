#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"
#include "transaction.h"
#include "utils.h"

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
