#include "queue.h"
#include "lfqueue.h"

#include <assert.h>
#include <stdlib.h>

struct queue* queue_create(){
    struct queue* queue = malloc(sizeof(*queue)); assert(queue);
    assert(lfqueue_init(&queue->lqueue) == 0);
    return queue;
}

void queue_push(struct queue* drpcq, void* el){
    assert(lfqueue_enq(&drpcq->lqueue,el) == 0);
}
void* queue_pop(struct queue* drpcq){
    return lfqueue_deq(&drpcq->lqueue);
}
void queue_free(struct queue* drpcq){
    lfqueue_destroy(&drpcq->lqueue);
    free(drpcq);
}
size_t queue_get_len(struct queue* drpcq){
    return lfqueue_size(&drpcq->lqueue);
}
