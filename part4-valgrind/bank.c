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
int pid;

#define PAGESIZE 4096

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

    sigset_t set; int sig; sigset_t set1; int sig1;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set1, SIGCONT);
    sigaddset(&set1, SIGUSR2);

    sigprocmask(SIG_BLOCK, &set, NULL);
    sigprocmask(SIG_BLOCK, &set1, NULL);


    FILE *f;
    if((f = fopen(argv[1], "r")) == NULL) { printf("Error opening file\n"); exit(1); }

    /*ACCOUNTS*/
    int num_accounts = read_num_accounts(f);

    void *shared_mem = mmap(NULL, (sizeof(account) * num_accounts), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    pid = fork();
    printf("PID: %d\n", pid);

    account *accounts = (account *) malloc(sizeof(account) * num_accounts); //array of accounts

    if(pid == 0) {
        // sigset_t set; int sig;
        // sigemptyset(&set);
        // sigaddset(&set, SIGUSR1);
        // sigaddset(&set, SIGCONT);
        printf("INIT PUDDLES\n");

        sigwait(&set, &sig);
        printf("PUDDLES READING FROM SHARED MEMORY\n");

        // printf("SHARED MEMORY\n%s\n", shared_mem);

        puddles_read_accounts(accounts, (char *) shared_mem, num_accounts);
        puddles_view_accounts(accounts, num_accounts);

        while(true) {
            sigwait(&set1, &sig1);
            if(sig1 == SIGUSR2) {
                printf("PUDDLES DONE\n");

                free_accounts(accounts, num_accounts);

                break;
            }
            
            puddles_issue_reward(accounts, num_accounts);

            puddles_view_accounts(accounts, num_accounts);
            // printf("PUDDLES WOKE UP\n");
        }
        
        // sleep(1);
        // puddles_read_accounts(accounts, f, num_accounts);
        // puddles_view_accounts(accounts, num_accounts);
    } else {
        // char *shared_string = (char *) shared_mem;
        read_accounts(accounts, f, num_accounts, (char *) shared_mem); //populate accounts
        // view_accounts(accounts, num_accounts);

        // printf("SHARED MEMORY\n%s\n", shared_mem);

        // printf("SIGNALLING PUDDLES\n");

        kill(pid, SIGUSR1);
    }

    if(pid == 0) {
        // sigset_t set; int sig;
        // sigemptyset(&set);
        // sigaddset(&set, SIGCONT);
        // sigaddset(&set, SIGUSR1);

        // sigprocmask(SIG_BLOCK, &set, NULL);
        //puddles logic
        // while(true) {
        //     sigwait(&set, &sig);
        //     printf("PUDDLES WOKE UP\n");
        //     if(sig == SIGCONT) {
        //         printf("PUDDLES SLEEPING AGAIN\n");
        //         printf("Parent pid: %d\n", getppid());
        //         // kill(getppid(), SIGUSR2);
        //     } else if(sig == SIGUSR1) {
        //         printf("PUDDLES EXITING\n");
        //         break;
        //     }
        // }
    } 
    else {
        // waitpid(0, NULL, 0);
        // return 0;
        /*TRANSACTIONS*/
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

        // printf("JOINING WORKER THREADS\n");
        
        //JOIN THREADS
        join_worker_threads(wts);

        join_bank_thread(bt);

        waitpid(0, NULL, 0);

        free_accounts(accounts, num_accounts);

        free_transactions_queue(tq);

        free_bank_thread(bt);
        free_worker_threads(wts);

        free(shared_mem);

        fclose(f);



        // wait(NULL);
    }

    //wait for child process to finish
    // if(pid != 0) {
    //     waitpid(0, NULL, 0);
    // }

    

    // // THE END
    // free_accounts(accounts, num_accounts);

    // free_transactions_queue(tq);

    // free_bank_thread(bt);
    // free_worker_threads(wts);

    // fclose(f);

    // printf("FINISHING MAIN\n");

    return 0;
}
