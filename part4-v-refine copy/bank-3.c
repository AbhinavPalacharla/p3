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
#include <sys/wait.h>

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

    // sigset_t set; int sig; sigset_t set1; int sig1;
    // sigemptyset(&set);
    // sigaddset(&set, SIGUSR1);
    // sigaddset(&set1, SIGCONT);
    // sigaddset(&set1, SIGUSR2);

    // sigprocmask(SIG_BLOCK, &set, NULL);
    // sigprocmask(SIG_BLOCK, &set1, NULL);

    FILE *f;
    if((f = fopen(argv[1], "r")) == NULL) { printf("Error opening file\n"); exit(1); }

    /*INIT SHARED MEM*/
    int num_accounts = read_num_accounts(f);

    void *shared_mem = mmap(NULL, (sizeof(account) * num_accounts), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    /*FORK PROCESS*/
    pid = fork(); //pid = 0 for child process, pid > 0 for parent process

    if(pid == 0) {
        /*PUDDLES BANK (CHILD PROCESS)*/
        printf("PUDDLES RUNNING...\n");

        /*SETUP SIGNALS*/
        sigset_t cont_set; int cont_sig; sigset_t stop_set; int stop_sig;

        sigemptyset(&cont_set); sigemptyset(&stop_set);

        sigaddset(&cont_set, SIGCONT); sigaddset(&cont_set, SIGUSR1);
        sigprocmask(SIG_BLOCK, &cont_set, NULL);

        sigaddset(&stop_set, SIGUSR2);

        /*INIT ACCOUNTS*/
        account *accounts = (account *) malloc(sizeof(account) * num_accounts);

        //Wait for duck bank to finish writing account info to shared memory
        sigwait(&cont_set, &cont_sig);

        //Copy account info from shared memory to local memory
        char *local_act_info = (char *) malloc(sizeof(account) * num_accounts);

        memcpy(local_act_info, shared_mem, sizeof(account) * num_accounts);

        printf("Local account info: \n");
        printf("%s\n", local_act_info);

        // //Free shared memory after copying it
        // munmap(shared_mem, sizeof(account) * num_accounts);

        // //Populate accounts
        // puddles_read_accounts(accounts, local_act_info, num_accounts);

        exit(0);
    } else {
        /*DUCK BANK (PARENT PROCESS)*/
        printf("DUCK RUNNING...\n");

        /*INIT ACCOUNTS*/
        account *accounts = (account *) malloc(sizeof(account) * num_accounts);

        read_accounts(accounts, f, num_accounts, (char *) shared_mem); //populate accounts and load into shared mem

        kill(pid, SIGUSR1); //signal puddles to read from shared mem

        sleep(10);
        /*THE END*/

        //Wait for puddles to finish and then exit
        waitpid(pid, NULL, 0);

        return 0;
    }
}
