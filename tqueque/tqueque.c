#include <pthread.h>
#include <stdlib.h>
#include "tqueque.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
struct tqueque* tqueque_create(){
    struct tqueque* tque = calloc(1,sizeof(*tque));
    assert(tque != NULL);
    pthread_mutex_init(&tque->lock,NULL);
    return tque;
}

void tqueque_push(struct tqueque* tque, void* el){
    assert(tque != NULL || el != NULL);
    pthread_mutex_lock(&tque->lock);
    struct tqueque_el* newel = calloc(1,sizeof(*newel));
    assert(newel != NULL);
    newel->prev = tque->ltop;
    if(tque->ltop != NULL)
        tque->ltop->next = newel;
    newel->ptr = el;
    tque->ltop = newel;
    if(tque->cur == NULL)
        tque->cur = tque->ltop;
    tque->len++;
    pthread_mutex_unlock(&tque->lock);
}

void* tqueque_pop(struct tqueque* tque){
    assert(tque != NULL);
    pthread_mutex_lock(&tque->lock);
    struct tqueque_el* cur = tque->cur;
    if(cur == NULL) {pthread_mutex_unlock(&tque->lock); return NULL;}
    if(tque->cur == tque->ltop)
        tque->ltop = NULL;
    void* next = tque->cur->next;
    void* out = tque->cur->ptr;
    if(tque->cur->next)
        tque->cur->next->prev = NULL;
    free(tque->cur);
    tque->cur = next;
    tque->len--;
    pthread_mutex_unlock(&tque->lock);
    return out;
}


uint64_t tqueque_get_len(struct tqueque* tque){
    assert(tque != NULL);
    pthread_mutex_lock(&tque->lock);
    struct tqueque_el* cur = tque->cur;
    ssize_t len = 0;
    while(cur){
            len++;
        cur = cur->next;
    }
    pthread_mutex_unlock(&tque->lock);
    return len;
}

void tqueque_free(struct tqueque* tque){
    if(tque == NULL)
        return;
    pthread_mutex_lock(&tque->lock);
    while(tque->cur){
        void *freep;
        freep = tque->cur;
        tque->cur = tque->cur->next;
        free(freep);
    }
    pthread_mutex_unlock(&tque->lock);
    pthread_mutex_destroy(&tque->lock);
    free(tque);
}

