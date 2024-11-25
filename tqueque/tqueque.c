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
    struct tqueque_el* new = malloc(sizeof(*new));assert(new);

    new->ptr = el;
    new->next = NULL;

    if(tque->ltop) tque->ltop->next = new;

    tque->ltop = new;

    if(tque->cur == NULL) tque->cur = tque->ltop;

    tque->len++;
    pthread_mutex_unlock(&tque->lock);
}

void* tqueque_pop(struct tqueque* tque){
    assert(tque != NULL);
    pthread_mutex_lock(&tque->lock);
    // struct tqueque_el* cur = tque->cur;
    // if(cur == NULL) {pthread_mutex_unlock(&tque->lock); return NULL;}
    // if(tque->cur == tque->ltop)
    //     tque->ltop = NULL;
    // void* next = tque->cur->next;
    // void* out = tque->cur->ptr;
    // if(tque->cur->next)
    //     tque->cur->next->prev = NULL;
    // free(tque->cur);
    // tque->cur = next;
    // tque->len--;
    if(tque->cur == NULL) {pthread_mutex_unlock(&tque->lock); return NULL;}
    char* out = tque->cur->ptr;
    void* freep = tque->cur;

    if(tque->cur == tque->ltop) tque->ltop = NULL;
    if(tque->cur) tque->cur = tque->cur->next;

    free(freep);
    tque->len--;
    pthread_mutex_unlock(&tque->lock);
    return out;
}


uint64_t tqueque_get_len(struct tqueque* tque){
    // assert(tque != NULL);
    // return tque->len;
    uint64_t len = 0;
    struct tqueque_el* cur = tque->cur;
    while(cur){cur = cur->next; len++;}
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
