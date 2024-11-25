#pragma once
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
struct tqueque_el{
    void* ptr;
    struct tqueque_el *next;
};

struct tqueque{
    pthread_mutex_t lock;
    struct tqueque_el* ltop;
    struct tqueque_el* cur;
    uint64_t len;
};
struct tqueque* tqueque_create();
void tqueque_push(struct tqueque* tque, void* el);
void* tqueque_pop(struct tqueque* tque);
void tqueque_free(struct tqueque* tque);
uint64_t tqueque_get_len(struct tqueque* tque);



