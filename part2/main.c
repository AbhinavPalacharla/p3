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

    int num_accounts = get_num_accounts(f);

    account *accounts = (account *) malloc(sizeof(account) * num_accounts); //array of accounts

    read_accounts(accounts, f, num_accounts); //populate accounts

    view_accounts(accounts, num_accounts);

    TransactionsQueue *tq = init_transactions_queue();

    size_t len = 128; char *line = malloc(sizeof(char) * len);  ssize_t read;

    //read transactions
    while((read = getline(&line, &len, f)) != -1) {
        tq->enqueue(tq, get_transaction(line));

        Transaction *t = tq->dequeue(tq);
        
        view_transaction(t);

        handle_transaction(t, accounts, num_accounts);
    }

    issue_reward(accounts, num_accounts);

    printf("\n");

    view_accounts(accounts, num_accounts);

    free(line);
    free_accounts(accounts, num_accounts);
    free_transactions_queue(tq);

    fclose(f);

    return 0;
}
