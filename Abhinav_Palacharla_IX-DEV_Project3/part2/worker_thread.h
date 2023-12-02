#ifndef WORKER_THREAD_H_
#define WORKER_THREAD_H_

#include <pthread.h>
#include "transaction.h"

typedef struct _ThreadHandlerArgs {
    TransactionQueue *tq;
    account *accounts;
    int num_accounts;
} ThreadHandlerArgs;

typedef struct _WorkerThread
{
    pthread_t thread;
    TransactionQueue *tq;
    ThreadHandlerArgs *args;
} WorkerThread;

WorkerThread *init_worker_threads(TransactionQueue *tq, account *accounts, int num_accounts);
void free_worker_threads(WorkerThread *wts);

#endif