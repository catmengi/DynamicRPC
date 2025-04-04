#pragma once
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
struct queue_el{
    void* ptr;
    struct queue_el *next;
};

struct queue{
    pthread_mutex_t lock;
    struct queue_el* ltop;
    struct queue_el* cur;
    size_t len;
};
struct queue* queue_create();
void queue_push(struct queue* drpcq, void* el);
void* queue_pop(struct queue* drpcq);

void queue_free_internals(struct queue* drpcq);
void queue_free(struct queue* drpcq);

size_t queue_get_len(struct queue* drpcq);

