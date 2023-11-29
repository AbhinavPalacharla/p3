#ifndef UTILS_H_
#define UTILS_H_

#include <string.h>
#include <pthread.h>
#include "account.h"

#define true 1
#define false 0
#define THREAD_POOL_SIZE 10
// #define NUM_TRANSACTIONS_PER_THREAD 12000
// #define THREAD_POOL_SIZE 5
// #define NUM_TRANSACTIONS_PER_THREAD 3

char *strip(char *str);

#endif