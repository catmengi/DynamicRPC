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

int tqueque_push(struct tqueque* tque, void* el, ssize_t len, char* tag){
    assert(tque != NULL || el != NULL);
    pthread_mutex_lock(&tque->lock);
    struct tqueque_el* newel = calloc(1,sizeof(*newel));
    assert(newel != NULL);
    if(tag)
        newel->tag = calloc(strlen(tag) + 1,sizeof(char));
    else
        newel->tag = NULL;
    if(tag)
        assert(newel->tag != NULL);
    newel->prev = tque->ltop;
    if(tque->ltop != NULL)
        tque->ltop->next = newel;
    newel->ptr = el;
    newel->len = len;
    if(newel->tag)
        strcpy(newel->tag,tag);
    tque->ltop = newel;
    if(tque->cur == NULL)
        tque->cur = tque->ltop;
    pthread_mutex_unlock(&tque->lock);
    return 0;
}

void* tqueque_pop(struct tqueque* tque, ssize_t* len, char* tag){
    assert(tque != NULL);
    pthread_mutex_lock(&tque->lock);
    struct tqueque_el* cur = tque->cur;
    if(cur == NULL) {pthread_mutex_unlock(&tque->lock); return NULL;}
    //*(int*)1 = 0;
    if(cur->tag == NULL || tag == NULL ||strcmp(cur->tag,tag) == 0){
        if(tque->cur == tque->ltop)
            tque->ltop = NULL;
        void* next = tque->cur->next;
        void* out = tque->cur->ptr;
        if(len)
            *len = tque->cur->len;
        if(tque->cur->next)
            tque->cur->next->prev = NULL;
        free(tque->cur->tag);
        free(tque->cur);
        tque->cur = next;
        pthread_mutex_unlock(&tque->lock);
        return out;
    }
    while(cur){
        if(cur->tag == NULL || tag == NULL ||strcmp(cur->tag,tag) == 0){
            if(cur == tque->ltop){
                tque->ltop = cur->prev;
            }
            void* out = cur->ptr;
            if(len)
                *len = cur->len;
            cur->prev->next = cur->next;
            if(cur->next)
                cur->next->prev = cur->prev;
            free(cur->tag);
            free(cur);
            pthread_mutex_unlock(&tque->lock);
            return out;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&tque->lock);
    return NULL;
}

void* tqueque_pop_wtag(struct tqueque* tque, ssize_t* len, char** tag){
    assert(tque != NULL);
    assert(tag);
    pthread_mutex_lock(&tque->lock);
    struct tqueque_el* cur = tque->cur;
    if(cur == NULL) {pthread_mutex_unlock(&tque->lock); return NULL;}
    void* out = cur->ptr;
    *tag = cur->tag;
    if(len != NULL)
        *len = cur->len;
    if(cur == tque->ltop)
        tque->ltop = NULL;
    tque->cur = tque->cur->next;
    free(cur);
    pthread_mutex_unlock(&tque->lock);
    return out;
}

ssize_t tqueque_get_tagamm(struct tqueque* tque, char* tag){
    assert(tque != NULL);
    pthread_mutex_lock(&tque->lock);
    struct tqueque_el* cur = tque->cur;
    ssize_t len = 0;
    if(tag == NULL){
        while(cur){
            cur = cur->next;
            len++;
        }
        pthread_mutex_unlock(&tque->lock);
        return len;
    }
    while(cur){
        if(cur->tag == NULL){
            len++;
        }else if(strcmp(cur->tag, tag) == 0){
            len++;
        }
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
        void *freep,*freet;
        freep = tque->cur;
        freet = tque->cur->tag;
        tque->cur = tque->cur->next;
        free(freep);
        free(freet);
    }
    pthread_mutex_unlock(&tque->lock);
    pthread_mutex_destroy(&tque->lock);
    free(tque);
}

