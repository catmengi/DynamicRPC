#pragma once
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
#include "lfqueue.h"

struct queue{
    lfqueue_t lqueue;
};

struct queue* queue_create();
void queue_push(struct queue* drpcq, void* el);
void* queue_pop(struct queue* drpcq);

void queue_free(struct queue* drpcq);

size_t queue_get_len(struct queue* drpcq);

