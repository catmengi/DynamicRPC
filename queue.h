#pragma once

#include <pthread.h>
#include <sys/types.h>
#include <assert.h>

#include "_bk_defines.h"
#include "impl_queue.h"
//generic drpc queue API interface

struct mutex_queue_s{
    queue que;
    pthread_mutex_t mutex;
};

typedef struct mutex_queue_s* queue_t;

/*inline*/ queue_t queue_create();
/*inline*/ void queue_push(queue_t q, void* val);
/*inline*/ void* queue_pop(queue_t q);
/*inline*/ size_t queue_count(queue_t q);
/*inline*/ void queue_free(queue_t q);


