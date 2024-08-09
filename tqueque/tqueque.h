#pragma once
#include <sys/types.h>
#include <pthread.h>
struct tqueque_el{
    char* tag;
    void* ptr;
    ssize_t len;
    struct tqueque_el *next,*prev;
};

struct tqueque{
    pthread_mutex_t lock;
    struct tqueque_el* ltop;
    struct tqueque_el* cur;
};
struct tqueque* tqueque_create();
int tqueque_push(struct tqueque* tque, void* el, ssize_t len, char* tag);
void* tqueque_pop(struct tqueque* tque, ssize_t* len, char* tag);
void* tqueque_pop_wtag(struct tqueque* tque, ssize_t* len, char** tag);           //instead of poping item with specified tag it pop any item and output tag
void tqueque_free(struct tqueque* tque);
ssize_t tqueque_get_tagamm(struct tqueque* tque, char* tag);
