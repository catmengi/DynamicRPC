#include <pthread.h>
#include <stdlib.h>
#include "queue.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
struct queue* queue_create(){
    struct queue* drpcq = calloc(1,sizeof(*drpcq));
    assert(drpcq != NULL);
    pthread_mutex_init(&drpcq->lock,NULL);
    return drpcq;
}

inline void queue_push(struct queue* drpcq, void* el){
    assert(drpcq != NULL || el != NULL);
    pthread_mutex_lock(&drpcq->lock);
    struct queue_el* new = malloc(sizeof(*new));assert(new);

    new->ptr = el;
    new->next = NULL;

    if(drpcq->ltop) drpcq->ltop->next = new;

    drpcq->ltop = new;

    if(drpcq->cur == NULL) drpcq->cur = drpcq->ltop;

    drpcq->len++;
    pthread_mutex_unlock(&drpcq->lock);
}

inline void queue_push_el(struct queue* drpcq, struct queue_el* new){
    assert(drpcq != NULL || new != NULL);

    new->next = NULL;

    if(drpcq->ltop) drpcq->ltop->next = new;

    drpcq->ltop = new;

    if(drpcq->cur == NULL) drpcq->cur = drpcq->ltop;

    drpcq->len++;
}

inline struct queue_el* queue_push_rp(struct queue* drpcq, void* el){
    assert(drpcq != NULL || el != NULL);
    struct queue_el* new = malloc(sizeof(*new));assert(new);

    new->ptr = el;
    new->next = NULL;

    if(drpcq->ltop) drpcq->ltop->next = new;

    drpcq->ltop = new;

    if(drpcq->cur == NULL) drpcq->cur = drpcq->ltop;

    drpcq->len++;
    return drpcq->ltop;
}

struct queue_el* queue_pop_el(struct queue* drpcq){
    assert(drpcq != NULL);
    if(drpcq->cur == NULL) {pthread_mutex_unlock(&drpcq->lock); return NULL;}
    struct queue_el* out = drpcq->cur;

    if(drpcq->cur == drpcq->ltop) drpcq->ltop = NULL;

    drpcq->cur = drpcq->cur->next;

    drpcq->len--;
    return out;
}

inline void* queue_pop(struct queue* drpcq){
    pthread_mutex_lock(&drpcq->lock);
    struct queue_el* pop = queue_pop_el(drpcq);
    if(pop == NULL) return NULL;

    void* ret = pop->ptr;

    free(pop);
    pthread_mutex_unlock(&drpcq->lock);
    return ret;
}


inline size_t queue_get_len(struct queue* drpcq){
    pthread_mutex_lock(&drpcq->lock);
    size_t ret = drpcq->len;
    pthread_mutex_unlock(&drpcq->lock);
    return ret;
}
void queue_free_internals(struct queue* drpcq){
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
void queue_free(struct queue* drpcq){
    queue_free_internals(drpcq);
    free(drpcq);
}
