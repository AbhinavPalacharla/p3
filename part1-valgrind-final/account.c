#define _GNU_SOURCE

#include "account.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include <pthread.h>

void free_accounts(account *accounts, int num_accounts) {
    for(int i = 0; i < num_accounts; i++) {
        pthread_mutex_destroy(&accounts[i].ac_lock);
    }

    free(accounts);
}

void view_accounts(account *accounts, int num_accounts)
{
    FILE *fp;

    if ((fp = fopen("./output.txt", "w+")) == NULL) {
        printf("ERROR: Couldn't open out.txt");
        exit(1);
    }

    printf("ACCOUNTS:\n\n");

    for (int i = 0; i < num_accounts; i++)
    {
        char *act_info;

        asprintf(&act_info, "%d balance: %0.2f\n", i, accounts[i].balance);
        
        fwrite(act_info, sizeof(char), strlen(act_info), fp);
        printf("%s", act_info);

        free(act_info);

        // printf("[%d/%d]\t (#) %s\t (PASSWD) %s\t (BAL) %0.2f\t RR %lf \n", i + 1, num_accounts, accounts[i].account_number, accounts[i].password, accounts[i].balance, accounts[i].reward_rate);
    }

    fclose(fp);

    printf("\n");
}

void read_accounts(account *accounts, FILE *f, int num_accounts)
{
    size_t len = 128;
    char *line = malloc(sizeof(char) * len);

    for (int i = 0; i < num_accounts; i++)
    {
        getline(&line, &len, f); // skip index line

        getline(&line, &len, f); // account number
        strcpy(accounts[i].account_number, strip(line));

        getline(&line, &len, f); // account password
        strcpy(accounts[i].password, strip(line));

        getline(&line, &len, f); // initial balance
        accounts[i].balance = atof(strip(line));

        getline(&line, &len, f); // reward rate
        accounts[i].reward_rate = strtod(strip(line), NULL);

        accounts[i].transaction_tracter = 0;

        pthread_mutex_init(&accounts[i].ac_lock, NULL);
    }

    free(line);
}

int find_account(char *account_number, account *accounts, int num_accounts)
{
    for (int i = 0; i < num_accounts; i++)
    {
        if (strcmp(account_number, accounts[i].account_number) == 0)
        {
            return i;
        }
    }

    return -1;
}

int authenticate_account(char *account_number, char *password, account *accounts, int num_accounts)
{
    for (int i = 0; i < num_accounts; i++)
    {
        if (strcmp(account_number, accounts[i].account_number) == 0)
        {
            if (strcmp(password, accounts[i].password) == 0)
            {
                return i;
            }
        }
    }

    printf("Failed to authenticate account %s | %s\n", account_number, password);

    return -1;
}

void issue_reward(account *accounts, int num_accounts)
{
    for (int i = 0; i < num_accounts; i++)
    {
        accounts[i].balance += accounts[i].transaction_tracter * accounts[i].reward_rate;
    }
}