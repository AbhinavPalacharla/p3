#ifndef WORKER_THREAD_H_
#define WORKER_THREAD_H_

#include <pthread.h>
#include "transaction.h"

typedef struct _WorkerThread
{
    pthread_t thread;
    TransactionQueue *tq;
} WorkerThread;

WorkerThread *init_worker_threads(TransactionQueue *tq, account *accounts, int num_accounts);

#endif