#include <pthread.h>
#include <stdlib.h>
#include "drpc_que.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
struct drpc_que* drpc_que_create(){
    struct drpc_que* drpcq = calloc(1,sizeof(*drpcq));
    assert(drpcq != NULL);
    pthread_mutex_init(&drpcq->lock,NULL);
    return drpcq;
}

void drpc_que_push(struct drpc_que* drpcq, void* el){
    assert(drpcq != NULL || el != NULL);
    pthread_mutex_lock(&drpcq->lock);
    struct drpc_que_el* new = malloc(sizeof(*new));assert(new);

    new->ptr = el;
    new->next = NULL;

    if(drpcq->ltop) drpcq->ltop->next = new;

    drpcq->ltop = new;

    if(drpcq->cur == NULL) drpcq->cur = drpcq->ltop;

    drpcq->len++;
    pthread_mutex_unlock(&drpcq->lock);
}

void drpc_que_push_el(struct drpc_que* drpcq, struct drpc_que_el* new){
    assert(drpcq != NULL || new != NULL);

    new->next = NULL;

    if(drpcq->ltop) drpcq->ltop->next = new;

    drpcq->ltop = new;

    if(drpcq->cur == NULL) drpcq->cur = drpcq->ltop;

    drpcq->len++;
}

struct drpc_que_el* drpc_que_push_rp(struct drpc_que* drpcq, void* el){
    assert(drpcq != NULL || el != NULL);
    struct drpc_que_el* new = malloc(sizeof(*new));assert(new);

    new->ptr = el;
    new->next = NULL;

    if(drpcq->ltop) drpcq->ltop->next = new;

    drpcq->ltop = new;

    if(drpcq->cur == NULL) drpcq->cur = drpcq->ltop;

    drpcq->len++;
    return drpcq->ltop;
}

struct drpc_que_el* drpc_que_pop_el(struct drpc_que* drpcq){
    assert(drpcq != NULL);
    if(drpcq->cur == NULL) {pthread_mutex_unlock(&drpcq->lock); return NULL;}
    struct drpc_que_el* out = drpcq->cur;

    if(drpcq->cur == drpcq->ltop) drpcq->ltop = NULL;

    drpcq->cur = drpcq->cur->next;

    drpcq->len--;
    return out;
}

void* drpc_que_pop(struct drpc_que* drpcq){
    pthread_mutex_lock(&drpcq->lock);
    struct drpc_que_el* pop = drpc_que_pop_el(drpcq);
    if(pop == NULL) return NULL;

    void* ret = pop->ptr;

    free(pop);
    pthread_mutex_unlock(&drpcq->lock);
    return ret;
}


uint64_t drpc_que_get_len(struct drpc_que* drpcq){
    pthread_mutex_lock(&drpcq->lock);
    uint64_t ret = drpcq->len;
    pthread_mutex_unlock(&drpcq->lock);
    return ret;
}
void drpc_que_free_internals(struct drpc_que* drpcq){
    if(drpcq == NULL)
        return;
    pthread_mutex_lock(&drpcq->lock);
    while(drpcq->cur){
        void *freep;
        freep = drpcq->cur;
        drpcq->cur = drpcq->cur->next;
        free(freep);
    }
    pthread_mutex_unlock(&drpcq->lock);
    pthread_mutex_destroy(&drpcq->lock);
}
void drpc_que_free(struct drpc_que* drpcq){
    drpc_que_free_internals(drpcq);
    free(drpcq);
}

