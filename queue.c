#include "queue.h"
#include "impl_queue.h"

/*inline*/ queue_t queue_create(){
    queue_t que = (queue_t)malloc(sizeof(*que));
    pthread_mutex_init(&que->mutex,NULL);
    que->que = impl_queue_init(sizeof(void*));
    return que;
}
/*inline*/ void queue_push(queue_t q, void* val){
    pthread_mutex_lock(&q->mutex);
    assert(impl_queue_push(q->que,&val) == BK_OK);
    pthread_mutex_unlock(&q->mutex);
}
/*inline*/ void* queue_pop(queue_t q){
    pthread_mutex_lock(&q->mutex);
    void* ret = NULL;

    impl_queue_pop(&ret,q->que);
    pthread_mutex_unlock(&q->mutex);
    return ret;
}
/*inline*/ size_t queue_count(queue_t q){
    return impl_queue_size(q->que);
}
/*inline*/ void queue_free(queue_t q){
    impl_queue_destroy(q->que);
    free(q);
}
