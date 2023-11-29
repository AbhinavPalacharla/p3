#define _GNU_SOURCE
#include "account.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void view_accounts(account *accounts, int num_accounts)
{
    printf("ACCOUNTS:\n\n");

    for (int i = 0; i < num_accounts; i++)
    {
        char *fpath;
        asprintf(&fpath, "act_%d", i);
        FILE *fp = fopen(fpath, "ab+");
        
        char *act_info;
        asprintf(&act_info, "[%d/%d]\t (#) %s\t (PASSWD) %s\t (BAL) %.2f\t RR %lf\t TRAC %lf \n", i, num_accounts - 1, accounts[i].account_number, accounts[i].password, accounts[i].balance, accounts[i].reward_rate, accounts[i].transaction_tracter);

        fwrite(act_info, sizeof(char), strlen(act_info), fp);
        printf("%s", act_info);

        free(fpath);
        free(act_info);

        fclose(fp);

        // printf("[%d/%d]\t (#) %s\t (PASSWD) %s\t (BAL) %.2f\t RR %lf \n", i + 1, num_accounts, accounts[i].account_number, accounts[i].password, accounts[i].balance, accounts[i].reward_rate);
    }

    printf("\n");
}

int read_num_accounts(FILE *f)
{
    size_t len = 128; char *line = malloc(sizeof(char) * len);

    getline(&line, &len, f);

    int num_accounts = atoi(line);

    free(line);

    return num_accounts;
}

void read_accounts(account *accounts, FILE *f, int num_accounts)
{
    size_t len = 128; char *line = malloc(sizeof(char) * len);

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

        if(pthread_mutex_init(&accounts[i].ac_lock, NULL) != 0) {
            printf("Error initializing mutex\n");
            exit(1);
        }
    }

    free(line);
}

void free_accounts(account *accounts, int num_accounts)
{
    for (int i = 0; i < num_accounts; i++)
    {
        pthread_mutex_destroy(&accounts[i].ac_lock);
    }

    free(accounts);
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

    // printf("Failed to authenticate account %s | %s\n", account_number, password);

    return -1;
}

void issue_reward(account *accounts, int num_accounts)
{
    for (int i = 0; i < num_accounts; i++)
    {
        accounts[i].balance += accounts[i].transaction_tracter * accounts[i].reward_rate;
        accounts[i].transaction_tracter = 0;
    }
}