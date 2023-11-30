#ifndef ACCOUNT_H_
#define ACCOUNT_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    char account_number[17];
    char password[9];
    double balance;
    double reward_rate;
    double transaction_tracter;
    char out_file[64];
    pthread_mutex_t ac_lock;
} account;

int read_num_accounts(FILE *f);

void view_accounts(account *accounts, int num_accounts);

void puddles_view_accounts(account *accounts, int num_accounts);

void read_accounts(account *accounts, FILE *f, int num_accounts, char *mem);

void puddles_read_accounts(account *accounts, char *mem, int num_accounts);

void free_accounts(account *accounts, int num_accounts);

int find_account(char *account_number, account *accounts, int num_accounts);

int authenticate_account(char *account_number, char *password, account *accounts, int num_accounts);

void issue_reward(account *accounts, int num_accounts);

#endif /* ACCOUNT_H_ */
