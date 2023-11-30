#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"
#include "transaction.h"
#include "utils.h"
#include <unistd.h>
#include "worker_thread.h"
#include "bank_thread.h"
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <wait.h>

pthread_barrier_t barrier;
int num_transactions_processed = 0;
pthread_mutex_t num_transactions_processed_mutex;
pthread_cond_t num_transactions_processed_cond;

pthread_barrier_t barrier1;
pthread_barrier_t barrier2;
pthread_barrier_t bank_barrier;
pthread_barrier_t exit_barrier;

int wakeup_bank_thread = 0;
pthread_mutex_t wakeup_bank_thread_mutex;
pthread_cond_t wakeup_bank_thread_cond;

int wakeup_worker_threads = 0;
pthread_mutex_t wakeup_worker_threads_mutex;
pthread_cond_t wakeup_worker_threads_cond;

pthread_mutex_t threads_running_mutex;
int num_threads_with_work = THREAD_POOL_SIZE;

pthread_mutex_t threads_waiting_for_bcast_mutex;
int num_threads_waiting_for_bcast = 0;

int thread_exit_flag = 0;

pid_t pid;

int main(int argc, char **argv) {
    pthread_barrier_init(&barrier, NULL, THREAD_POOL_SIZE);
    pthread_barrier_init(&barrier1, NULL, THREAD_POOL_SIZE);
    pthread_barrier_init(&barrier2, NULL, THREAD_POOL_SIZE);
    pthread_barrier_init(&bank_barrier, NULL, THREAD_POOL_SIZE);
    pthread_barrier_init(&exit_barrier, NULL, THREAD_POOL_SIZE);

    pthread_mutex_init(&num_transactions_processed_mutex, NULL);
    pthread_cond_init(&num_transactions_processed_cond, NULL);

    pthread_mutex_init(&wakeup_bank_thread_mutex, NULL);
    // pthread_mutex_lock(&wakeup_bank_thread_mutex);
    pthread_cond_init(&wakeup_bank_thread_cond, NULL);

    pthread_mutex_init(&wakeup_worker_threads_mutex, NULL);
    // pthread_mutex_lock(&wakeup_worker_threads_mutex);
    pthread_cond_init(&wakeup_worker_threads_cond, NULL);

    /*SETUP SIGNALS*/
    sigset_t start_set; int start_sig; sigset_t while_set; int while_sig;

    sigemptyset(&start_set); sigemptyset(&while_set);

    sigaddset(&start_set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &start_set, NULL);

    sigaddset(&while_set, SIGCONT); sigaddset(&while_set, SIGUSR2);
    sigprocmask(SIG_BLOCK, &while_set, NULL);

    FILE *f;
    if((f = fopen(argv[1], "r")) == NULL) { printf("Error opening file\n"); exit(1); }

    /*INIT SHARED MEM*/
    int num_accounts = read_num_accounts(f);

    void *shared_mem = mmap(NULL, (sizeof(account) * num_accounts), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    /*FORK PROCESS*/
    pid = fork(); //pid = 0 for child process, pid > 0 for parent process

    if(pid < 0) {
        printf("Error forking process\n");
        exit(1);
    }

    printf("pid: %d\n", pid);

    if(pid == 0) {
        printf("PUDDLES\n");

        /*INIT ACCOUNTS*/
        account *accounts = (account *) malloc(sizeof(account) * num_accounts);

        //Wait for duck bank to finish writing account info to shared memory
        sigwait(&start_set, &start_sig);

        printf("Signal received: %d\n", start_sig);

        // Copy account info from shared memory to local memory
        char *local_act_info = (char *) malloc(sizeof(account) * num_accounts);

        memcpy(local_act_info, shared_mem, sizeof(account) * num_accounts);

        // printf("Local account info: \n");
        // printf("%s\n", local_act_info);

        //Free shared memory after copying it
        munmap(shared_mem, sizeof(account) * num_accounts);

        //Populate accounts
        puddles_read_accounts(accounts, local_act_info, num_accounts);
        // free(local_act_info);

        puddles_view_accounts(accounts, num_accounts);

        // printf("PUDDLES ACCOUNTS: \n");
        // print_accounts(accounts, num_accounts);

        sigwait(&while_set, &while_sig);
        printf("PUDDLES RECIEVED SIG FROM DUCK BANK THREAD\n");

        while(true) {
            sigwait(&while_set, &while_sig);
            
            if(while_sig == SIGUSR2) {
                printf("PUDDLES DONE\n");
                
                puddles_issue_reward(accounts, num_accounts);

                puddles_view_accounts(accounts, num_accounts);

                break;
            }
            
            printf("PUDDLES ISSUING REWARD\n");
            puddles_issue_reward(accounts, num_accounts);

            puddles_view_accounts(accounts, num_accounts);
        }

        free_accounts(accounts, num_accounts);


        // fclose(f);

        exit(0);
    }
    else {
        printf("DUCK\n");

        /*INIT ACCOUNTS*/
        account *accounts = (account *) malloc(sizeof(account) * num_accounts);

        read_accounts(accounts, f, num_accounts, (char *) shared_mem); //populate accounts and load into shared mem

        kill(pid, SIGUSR1); //signal puddles to read from shared mem

        /*LOAD TRANSACTIONS*/
        TransactionQueue *tq = init_transaction_queue();

        size_t len = 128; char *line = malloc(sizeof(char) * len);  ssize_t read;

        while((read = getline(&line, &len, f)) != -1) {
            tq->enqueue(tq, read_transaction(line));
        }

        free(line);

        /*BANK THREAD*/
        BankThread *bt = init_bank_thread(accounts, num_accounts);

        /*WORKER THREADS*/
        WorkerThread *wts = init_worker_threads(tq, accounts, num_accounts);

        /*JOIN THREADS*/
        join_worker_threads(wts);
        join_bank_thread(bt);

        /*THE END*/
        int status;
        if(waitpid(pid, &status, 0) > 0) {
            if(WIFEXITED(status)) {
                printf("PUDDLES EXITED WITH STATUS: %d\n", WEXITSTATUS(status));
            }
            else {
                printf("PUDDLES DIDN'T EXIT PROPERLY.\n");
            }
        }
        else {
            printf("Error waiting for puddles to exit\n");
        }

        // if (WIFEXITED(status)) {
        //     printf("PUDDLES EXITED WITH STATUS: %d\n", WEXITSTATUS(status));
        // } else {
        //     printf("PUDDLES DIDN'T EXIT PROPERLY.\n");
        // }

        printf("DUCK EXITING\n");
    }

    return 0;

}

    // if(pid == 0) {
    //     /*PUDDLES BANK (CHILD PROCESS)*/
    //     printf("PUDDLES RUNNING...\n");

    //     /*SETUP SIGNALS*/
    //     sigset_t cont_set; int cont_sig; sigset_t stop_set; int stop_sig;

    //     sigemptyset(&cont_set); sigemptyset(&stop_set);

    //     sigaddset(&cont_set, SIGCONT); sigaddset(&cont_set, SIGUSR1);
    //     sigprocmask(SIG_BLOCK, &cont_set, NULL);

    //     sigaddset(&stop_set, SIGUSR2);

    //     exit(0);

    //     // /*INIT ACCOUNTS*/
    //     // account *accounts = (account *) malloc(sizeof(account) * num_accounts);

    //     // //Wait for duck bank to finish writing account info to shared memory
    //     // sigwait(&cont_set, &cont_sig);

    //     // //Copy account info from shared memory to local memory
    //     // char *local_act_info = (char *) malloc(sizeof(account) * num_accounts);

    //     // memcpy(local_act_info, shared_mem, sizeof(account) * num_accounts);

    //     // printf("Local account info: \n");
    //     // printf("%s\n", local_act_info);
        

    // } else {
    //     /*DUCK BANK (PARENT PROCESS)*/
    //     printf("DUCK RUNNING...\n");

    //     /*INIT ACCOUNTS*/
    //     // account *accounts = (account *) malloc(sizeof(account) * num_accounts);

    //     // read_accounts(accounts, f, num_accounts, (char *) shared_mem); //populate accounts and load into shared mem

    //     kill(pid, SIGUSR1); //signal puddles to read from shared mem

    //     printf("DUCK WAITING...\n");
    //     waitpid(0, NULL, 0);
    //     printf("DUCK EXITING\n");
    // }

    // return 0;

