#pragma once
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
struct drpc_que_el{
    void* ptr;
    struct drpc_que_el *next;
};

struct drpc_que{
    pthread_mutex_t lock;
    struct drpc_que_el* ltop;
    struct drpc_que_el* cur;
    uint64_t len;
};
struct drpc_que* drpc_que_create();
void drpc_que_push(struct drpc_que* drpcq, void* el);
void* drpc_que_pop(struct drpc_que* drpcq);

void drpc_que_free_internals(struct drpc_que* drpcq);
void drpc_que_free(struct drpc_que* drpcq);

uint64_t drpc_que_get_len(struct drpc_que* drpcq);



