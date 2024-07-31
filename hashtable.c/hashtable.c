#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#define ALLOC_ERR 2
#define ARG_ERR 1
#define ENOTFOUND 4
#define ENOSPACE 3
#define _GNU_SOURCE

struct ht_llist{
    uint64_t hash;

    struct ht_entry* entry;
    struct ht_entry* last_acc;

    struct ht_llist* next;
    struct ht_llist* prev;
};

struct ht_entry{
    char* key;
    uint32_t keylen;

    void* data;
    uint64_t meta;

    struct ht_entry* next;
    struct ht_entry* prev;
    struct ht_llist* parent;
};

struct ht_llist_cont{
    struct ht_llist* llist;    //list of key/value pairs
    struct ht_llist_cont* next; //separate chaining
    struct ht_llist_cont* prev;
    struct ht_bucket* parent;
};

struct ht_bucket{
    struct ht_llist_cont* llist_cont;  //pointer to container
    struct ht_bucket* next;  //pointer to next bucket(only valid if full is true)
    struct ht_bucket* prev;
    uint32_t occup;
    bool full;    //if true we go to the "next" or try to found next free bucket but if hashtable is full we try to rehash;
    bool freed;
};

struct hashtable{
    struct ht_bucket* bucket;
    struct ht_llist* llist_head;
    struct ht_llist* llist_tail;
    uint32_t bucket_amm;
    uint32_t coll_pbuck;
    uint32_t empty;
    uint32_t rehashes;
};

uint64_t _hash_fnc(char* str,uint32_t keylen){
  uint64_t h = (525201411107845655ull);
  for (int i =0; i < keylen; i++,str++){
        h ^= *str;
        h *= 0x5bd1e9955bd1e995;
        h ^= h >> 47;
  }
  return h;
}

int hashtable_create(struct hashtable** ht,size_t size,size_t coll_pbuck)
{
    if(!ht)
        return ARG_ERR;
    *ht= calloc(1,sizeof(*(*ht)));
    if(!*ht)
        return ALLOC_ERR;
    (*ht)->bucket_amm = size;
    (*ht)->coll_pbuck = coll_pbuck;
    (*ht)->empty = (*ht)->bucket_amm -1;
    (*ht)->bucket = calloc((*ht)->bucket_amm,sizeof(*(*ht)->bucket));
    if(!(*ht)->bucket)
        return ALLOC_ERR;
    (*ht)->llist_head = calloc(1,sizeof(*(*ht)->llist_head));
    if(!(*ht)->llist_head)
        return ALLOC_ERR;
    (*ht)->llist_tail = (*ht)->llist_head;
    return 0;
}
int hashtable_rehash(struct hashtable* ht,uint64_t size);
int hashtable_add(struct hashtable* ht,char* key,uint32_t keylen,void* vptr,uint64_t meta)
{
    if(!ht)
        return ARG_ERR;
    if(!key)
        return ARG_ERR;
    uint64_t hash = _hash_fnc(key,keylen);
    uint64_t index = hash % ht->bucket_amm;
    struct ht_llist* tmp_l = NULL;
    struct ht_llist_cont* tmp_c = NULL;
    struct ht_bucket* bucket = NULL;
    struct ht_entry* tmp_e = NULL;
    if(ht->empty == 0){
        int ret = 0;
        ret = hashtable_rehash(ht,ht->bucket_amm + (ht->bucket_amm * 2));
        while(ret == ENOSPACE){
            ret = hashtable_rehash(ht,ht->bucket_amm + (ht->bucket_amm * 2));
        }
        if(ret != 0) return ret;
        index = hash % ht->bucket_amm;
        if(ht->rehashes % 10 == 0)
            ht->coll_pbuck++;
        ht->rehashes++;
        //todo rehash!!!!
    }
    if(ht->bucket[index].full == true){
        bucket = &ht->bucket[index];
        for(int i = index; i < ht->bucket_amm && i != (index - 1); i++){
            while(bucket->next != NULL && bucket->full == true) bucket = bucket->next;

            if(bucket->freed == true){
                bucket->freed = false;
            }
            if(bucket->full == false) break;

            if(ht->bucket[i].full == false && ht->bucket[i].prev == NULL){
                bucket->next = &ht->bucket[i];
                bucket->next->prev = bucket;
            }

            if(i == ht->bucket_amm - 1) i = 0;
        }
    }else{
        bucket = &ht->bucket[index];
    }
    if(bucket->llist_cont == NULL){
        if(bucket->freed == true)
            bucket->freed = false;
        //printf("new bucket or empty bucket\n");
        bucket->llist_cont = calloc(1,sizeof(*bucket->llist_cont));
        if(!bucket->llist_cont)
            return ALLOC_ERR;
        if(ht->llist_tail->entry == NULL)
            tmp_l = ht->llist_tail;
        else{
            tmp_l = calloc(1,sizeof(*tmp_l));
            if(!tmp_l)
                return ALLOC_ERR;
            tmp_l->prev = ht->llist_tail;
            ht->llist_tail->next = tmp_l;
            ht->llist_tail = tmp_l;
        }
        tmp_l->entry = calloc(1,sizeof(*tmp_l->entry));
        if(!tmp_l->entry)
            return ALLOC_ERR;
        tmp_l->hash = hash;
        tmp_e = tmp_l->entry;
        tmp_e->parent = tmp_l;
        tmp_e->data = vptr;
        tmp_e->key = strdup(key);
        tmp_e->keylen = keylen;
        tmp_e->meta = meta;
        bucket->llist_cont->llist = tmp_l;
        bucket->llist_cont->parent = bucket;
        ht->empty--;
        bucket->occup++;
        if(bucket->occup == ht->coll_pbuck)
            bucket->full = true;
        return 0;
    }else{
        tmp_c = bucket->llist_cont;
        while(tmp_c->next != NULL && tmp_c->llist->hash != hash) tmp_c = tmp_c->next;
        if(tmp_c->llist->hash == hash){
            tmp_l = tmp_c->llist;
            tmp_e = tmp_l->entry;
            while(tmp_e->next != NULL && strcmp(tmp_e->key , key) != 0){
                tmp_e = tmp_e->next;
            }
            if(strcmp(tmp_e->key,key) == 0){
                //printf("key overwrite %s %s\n",key,tmp_e->key);
                tmp_e->data = vptr;
                tmp_e->keylen = keylen;
                tmp_e->meta = meta;
                return 0;
            }
            //printf("key collision\n");
            tmp_e->next = calloc(1,sizeof(*tmp_e->next));
            if(!tmp_e->next)
                return ALLOC_ERR;
            tmp_e->next->parent = tmp_l;
            tmp_e->next->prev = tmp_e;
            tmp_e = tmp_e->next;
            tmp_e->data = vptr;
            tmp_e->key = strdup(key);
            tmp_e->keylen = keylen;
            tmp_e->meta = meta;
            return 0;
        }else{
           // printf("bucket collide 2\n");
            while(tmp_c->next != NULL) tmp_c = tmp_c->next;
            tmp_c->next = calloc(1,sizeof(*tmp_c->next));
            if(!tmp_c->next) return ALLOC_ERR;
            tmp_c->next->prev = tmp_c;
            tmp_c = tmp_c->next;
            tmp_l = calloc(1,sizeof(*tmp_l));
            if(!tmp_l)
                return ALLOC_ERR;
            ht->llist_tail->next = tmp_l;
            tmp_l->prev = ht->llist_tail;
            ht->llist_tail = tmp_l;
            tmp_l->entry = calloc(1,sizeof(*tmp_l->entry));
            if(!tmp_l->entry) return ALLOC_ERR;
            tmp_l->hash = hash;
            tmp_c->llist = tmp_l;
            tmp_c->parent = bucket;
            tmp_e = tmp_l->entry;
            tmp_e->prev = NULL;
            tmp_e->parent = tmp_l;
            tmp_e->data = vptr;
            tmp_e->key = strdup(key);
            tmp_e->keylen = keylen;
            tmp_e->meta = meta;
            bucket->occup++;
            if(bucket->occup == ht->coll_pbuck) bucket->full = true;
            return 0;
        }
    }
}

int hashtable_rehash(struct hashtable* ht,uint64_t size)
{
    if(!ht) return ARG_ERR;
    for(int i = 0; i < ht->bucket_amm; i++){
    struct ht_bucket* bucket = &ht->bucket[i];
    struct ht_llist_cont* tmp_c  = bucket->llist_cont;
        while(tmp_c){
            struct ht_llist_cont* n = tmp_c->next;
            free(tmp_c);
            tmp_c = n;
        }
    }
    struct ht_bucket* nptr = realloc(ht->bucket,size*sizeof(struct ht_bucket));
    if(nptr)
        ht->bucket = nptr;
    else {;return ALLOC_ERR;}
    memset(ht->bucket,0,size*sizeof(struct ht_bucket));
    ht->bucket_amm = size;
    ht->empty = size;
    ht->rehashes++;
    struct ht_llist* tmp_l = ht->llist_head;
    struct ht_bucket* bucket = NULL;
    uint64_t index = 0;
    while(tmp_l){
        if(ht->empty == 0) {return ENOSPACE;}
        index =  tmp_l->hash % ht->bucket_amm;
        bucket = &ht->bucket[index];
        if(bucket->full == true){
            for(int i = index; i < ht->bucket_amm && i != (index - 1); i++){
                printf("%s: doing rehash\n",__PRETTY_FUNCTION__);
                while(bucket->next != NULL && bucket->full == true) bucket = bucket->next;

                if(bucket->full == false) break;

                if(ht->bucket[i].full == false && ht->bucket[i].prev == NULL){
                    bucket->next = &ht->bucket[i];
                    bucket->next->prev = bucket;
                }
                if(i == ht->bucket_amm - 1) i = 0;
            }
        }
        if(bucket->llist_cont == NULL){
            bucket->llist_cont = calloc(1,sizeof(*bucket->llist_cont));
            if(!bucket->llist_cont) return ALLOC_ERR;
            bucket->llist_cont->llist = tmp_l;
            bucket->llist_cont->parent = bucket;
            bucket->occup++;
            ht->empty--;
        }
        else{
            struct ht_llist_cont* tmp_c = bucket->llist_cont;
            while(tmp_c->next != NULL) tmp_c = tmp_c->next;
            tmp_c->next = calloc(1,sizeof(*tmp_c->next));
            if(!tmp_c->next) return ALLOC_ERR;
            tmp_c->next->prev = tmp_c;
            tmp_c = tmp_c->next;
            tmp_c->parent = bucket;
            tmp_c->llist = tmp_l;
            bucket->occup++;
            if(bucket->occup == ht->coll_pbuck)
                bucket->full = true;
        }
        tmp_l = tmp_l->next;
    }
    return 0;
}

int hashtable_get(struct hashtable* ht, char* key, uint32_t keylen, void** out)
{
    if(!ht)
        return ARG_ERR;
    if(!key)
        return ARG_ERR;
    struct ht_bucket* bucket = NULL;
    struct ht_llist_cont* tmp_c = NULL;
    struct ht_entry* tmp_e = NULL;
    struct ht_llist* tmp_l = NULL;
    uint64_t hash = _hash_fnc(key,keylen);
    uint64_t index = hash % ht->bucket_amm;
    bucket = &ht->bucket[index];
    while(bucket){
        if(bucket->freed == true){
            bucket = bucket->next;
            continue;
        }
        if(bucket->llist_cont != NULL)
            tmp_c = bucket->llist_cont;
        else{
            bucket = bucket->next;
            continue;
        }
        while(tmp_c){
            tmp_l = tmp_c->llist;
            if(tmp_l->last_acc){
                if(tmp_l->last_acc->parent->hash == hash && strncmp(tmp_l->last_acc->key,key,tmp_l->last_acc->keylen) == 0){
                    *out = tmp_l->last_acc->data;
                    return 0;
                }
            }
            if(tmp_l->hash == hash){
                tmp_e = tmp_l->entry;
                while(tmp_e){
                    if(tmp_e->keylen == keylen){
                        if(strncmp(key,tmp_e->key,tmp_e->keylen) == 0){
                            *out = tmp_e->data;
                            tmp_l->last_acc = tmp_e;
                            return 0;
                        }
                    }
                    tmp_e = tmp_e->next;
                }
            }
            tmp_c = tmp_c->next;
        }
        bucket = bucket->next;
    }
    return ENOTFOUND;
}

int hashtable_get_by_hash(struct hashtable* ht, uint64_t hash, void** out)
{
    if(!ht)
        return ARG_ERR;
    struct ht_bucket* bucket = NULL;
    struct ht_llist_cont* tmp_c = NULL;
    struct ht_entry* tmp_e = NULL;
    struct ht_llist* tmp_l = NULL;
    uint64_t index = hash % ht->bucket_amm;
    bucket = &ht->bucket[index];
    while(bucket){
        if(bucket->freed == true){
            bucket = bucket->next;
            continue;
        }
        if(bucket->llist_cont != NULL)
            tmp_c = bucket->llist_cont;
        else{
            bucket = bucket->next;
            continue;
        }
        while(tmp_c){
            tmp_l = tmp_c->llist;
            if(tmp_l->last_acc){
                if(tmp_l->last_acc->parent->hash == hash){
                    *out = tmp_l->last_acc->data;
                    return 0;
                }
            }
            if(tmp_l->hash == hash){
                tmp_e = tmp_l->entry;
                *out = tmp_e->data;
                tmp_l->last_acc = tmp_e;
                return 0;
            }
        tmp_c = tmp_c->next;
    }
        bucket = bucket->next;
    }
    return ENOTFOUND;
}
int hashtable_getwmeta(struct hashtable* ht, char* key, uint32_t keylen, void** out,uint64_t* meta)
{
    if(!ht)
        return ARG_ERR;
    if(!key)
        return ARG_ERR;
    struct ht_bucket* bucket = NULL;
    struct ht_llist_cont* tmp_c = NULL;
    struct ht_entry* tmp_e = NULL;
    struct ht_llist* tmp_l = NULL;
    uint64_t hash = _hash_fnc(key,keylen);
    uint64_t index = hash % ht->bucket_amm;
    bucket = &ht->bucket[index];
    while(bucket){
        if(bucket->freed == true){
            bucket = bucket->next;
            continue;
        }
        if(bucket->llist_cont != NULL)
            tmp_c = bucket->llist_cont;
        else{
            bucket = bucket->next;
            continue;
        }
        while(tmp_c){
            tmp_l = tmp_c->llist;
            if(tmp_l->last_acc){
                if(tmp_l->last_acc->parent->hash == hash && strncmp(tmp_l->last_acc->key,key,tmp_l->last_acc->keylen) == 0){
                    *out = tmp_l->last_acc->data;
                    *meta = tmp_l->last_acc->meta;
                    return 0;
                }
            }
            if(tmp_l->hash == hash){
                tmp_e = tmp_l->entry;
                while(tmp_e){
                    if(tmp_e->keylen == keylen){
                        if(strncmp(key,tmp_e->key,tmp_e->keylen) == 0){
                            *out = tmp_e->data;
                            *meta = tmp_e->meta;
                            tmp_l->last_acc = tmp_e;
                            return 0;
                        }
                    }
                    tmp_e = tmp_e->next;
                }
            }
            tmp_c = tmp_c->next;
        }
        bucket = bucket->next;
    }
    return ENOTFOUND;
}
int hashtable_get_key(struct hashtable* ht, char* key, uint32_t keylen, char** out)
{
    if(!ht)
        return ARG_ERR;
    if(!key)
        return ARG_ERR;
    struct ht_bucket* bucket = NULL;
    struct ht_llist_cont* tmp_c = NULL;
    struct ht_entry* tmp_e = NULL;
    struct ht_llist* tmp_l = NULL;
    uint64_t hash = _hash_fnc(key,keylen);
    uint64_t index = hash % ht->bucket_amm;
    bucket = &ht->bucket[index];
    while(bucket){
        if(bucket->freed == true){
            bucket = bucket->next;
            continue;
        }
        if(bucket->llist_cont != NULL)
            tmp_c = bucket->llist_cont;
        else{
            bucket = bucket->next;
            continue;
        }
        while(tmp_c){
            tmp_l = tmp_c->llist;
            if(tmp_l->last_acc){
                if(tmp_l->last_acc->parent->hash == hash && strncmp(tmp_l->last_acc->key,key,tmp_l->last_acc->keylen) == 0){
                    *out = tmp_l->last_acc->key;
                    return 0;
                }
            }
            if(tmp_l->hash == hash){
                tmp_e = tmp_l->entry;
                while(tmp_e){
                    if(tmp_e->keylen == keylen){
                        if(strncmp(key,tmp_e->key,tmp_e->keylen) == 0){
                            *out = tmp_e->key;
                            tmp_l->last_acc = tmp_e;
                            return 0;
                        }
                    }
                    tmp_e = tmp_e->next;
                }
            }
            tmp_c = tmp_c->next;
        }
        bucket = bucket->next;
    }
    return ENOTFOUND;
}

int _hashtable_get_bucket(struct hashtable* ht, char* key, uint32_t keylen, struct ht_bucket** out)
{
    if(!ht)
        return ARG_ERR;
    if(!key)
        return ARG_ERR;
    struct ht_bucket* bucket = NULL;
    struct ht_llist_cont* tmp_c = NULL;
    struct ht_entry* tmp_e = NULL;
    struct ht_llist* tmp_l = NULL;
    uint64_t hash = _hash_fnc(key,keylen);
    uint64_t index = hash % ht->bucket_amm;
    bucket = &ht->bucket[index];
    while(bucket){
        if(bucket->freed == true){
            bucket = bucket->next;
            continue;
        }
        if(bucket->llist_cont != NULL)
            tmp_c = bucket->llist_cont;
        else{
            bucket = bucket->next;
            continue;
        }
        tmp_l = tmp_c->llist;
        while(tmp_c){
            if(tmp_l->hash == hash){
                tmp_e = tmp_l->entry;
                while(tmp_e){
                    if(tmp_e->keylen == keylen){
                        if(strncmp(key,tmp_e->key,tmp_e->keylen) == 0){
                            *out = bucket;
                            return 0;
                        }
                    }
                    tmp_e = tmp_e->next;
                }
            }
            tmp_c = tmp_c->next;
        }
        bucket = bucket->next;
    }
    return ENOTFOUND;
}

int _hashtable_get_cont_from_bucket(struct ht_bucket* bucket, char* key, uint32_t keylen, struct ht_llist_cont** out)
{
    if(!bucket)
        return ARG_ERR;
    if(!key)
        return ARG_ERR;
    struct ht_llist_cont* tmp_c = NULL;
    struct ht_entry* tmp_e = NULL;
    struct ht_llist* tmp_l = NULL;
    uint64_t hash = _hash_fnc(key,keylen);
    while(bucket){
        if(bucket->freed == true){
            bucket = bucket->next;
            continue;
        }
        if(bucket->llist_cont != NULL)
            tmp_c = bucket->llist_cont;
        else{
            bucket = bucket->next;
            continue;
        }
        while(tmp_c){
            tmp_l = tmp_c->llist;
            if(tmp_l->hash == hash){
                tmp_e = tmp_l->entry;
                while(tmp_e){
                    if(tmp_e->keylen == keylen){
                        if(strncmp(key,tmp_e->key,tmp_e->keylen) == 0){
                            *out = tmp_c;
                            return 0;
                        }
                    }
                    tmp_e = tmp_e->next;
                }
            }
            tmp_c = tmp_c->next;
        }
        bucket = bucket->next;
    }
    return ENOTFOUND;
}

int _hashtable_get_entry_from_ll(struct ht_llist* tmp_l, char* key, uint32_t keylen, struct ht_entry** out)
{
    if(!tmp_l)
        return ARG_ERR;
    if(!key)
        return ARG_ERR;
    struct ht_entry* tmp_e = NULL;
    uint64_t hash = _hash_fnc(key,keylen);
            if(tmp_l->hash == hash){
                tmp_e = tmp_l->entry;
                while(tmp_e){
                    if(tmp_e->keylen == keylen){
                        if(strncmp(key,tmp_e->key,tmp_e->keylen) == 0){
                            *out = tmp_e;
                            return 0;
                        }
                    }
                    tmp_e = tmp_e->next;
                }
            }
    return ENOTFOUND;
}

int hashtable_remove_entry(struct hashtable* ht,char* key,uint32_t keylen)
{
    if(!key) return ARG_ERR;
    if(!ht) return ARG_ERR;
    struct ht_bucket* bucket =  NULL;
    int ret = _hashtable_get_bucket(ht,key,keylen,&bucket);
    if(ret != 0) return ret;
    struct ht_llist_cont* tmp_c = NULL;
    struct ht_llist* tmp_l = NULL;
    ret = _hashtable_get_cont_from_bucket(bucket,key,keylen,&tmp_c);
    if(ret != 0) return ret;
    if(tmp_c == NULL) return ARG_ERR;
    if(!bucket) return ENOTFOUND;
    tmp_l = tmp_c->llist;
    struct ht_entry* tmp_e = NULL;
    int eret = _hashtable_get_entry_from_ll(tmp_l,key,keylen,&tmp_e);
    if(eret != 0) return eret;
    if(tmp_e->next == NULL && tmp_e->prev == NULL){
        if(tmp_l->next && tmp_l->prev){
            tmp_l->prev->next = tmp_l->next;
            tmp_l->next->prev = tmp_l->prev;
            free(tmp_l);
            free(tmp_e->key);free(tmp_e);
            goto LINK;
        }
        if(tmp_l->next && tmp_l->prev == NULL){
            ht->llist_head = tmp_l->next;
            ht->llist_head->prev = NULL;
            free(tmp_l);
            free(tmp_e->key);free(tmp_e);
            goto LINK;
        }
        if(tmp_l->next == NULL && tmp_l->prev){
            tmp_l->prev->next = NULL;
            ht->llist_tail = tmp_l->prev;
            free(tmp_e->key);free(tmp_e);
            free(tmp_l);
            goto LINK;
        }
        if(tmp_l->next == NULL && tmp_l->prev == NULL){
            memset(tmp_l,0,sizeof(*tmp_l));
            free(tmp_e->key);free(tmp_e);
            goto LINK;
        }
        LINK:
        if(tmp_c->next && tmp_c->prev){
            tmp_c->prev->next = tmp_c->next;
            tmp_c->next->prev = tmp_c->prev;
            bucket->occup--;
            free(tmp_c);
            return 0;
        }
        if(tmp_c->prev == NULL && tmp_c->next){
            bucket->llist_cont = tmp_c->next;
            tmp_c->next->prev = NULL;
            bucket->occup--;
            free(tmp_c);
            return 0;
        }
        if(tmp_c->prev && tmp_c->next == NULL){
            tmp_c->prev->next = NULL;
            bucket->occup--;
            free(tmp_c);
            return 0;
        }
        if(tmp_c->prev == NULL && tmp_c->next == NULL){
            bucket->llist_cont = NULL;
            free(tmp_c);
            if(bucket->next && bucket->prev){
                struct ht_bucket* tmp_b_n = bucket->next;
                struct ht_bucket* tmp_b_p = bucket->prev;
                bucket->prev->next = bucket->next;
                bucket->next->prev = bucket->prev;
                memset(bucket,0,sizeof(*bucket));
                bucket->prev = tmp_b_p;    //dirty search bug fix
                bucket->next = tmp_b_n;
                bucket->freed  = true;
                ht->empty++;
                return 0;
            }
            if(bucket->next && !bucket->prev){
                bucket->freed = true;
                bucket->full = false;
                ht->empty++;
                bucket->occup = 0;
                return 0;
            }
            if(!bucket->next && bucket->prev){
                bucket->prev->next = NULL;
                ht->empty++;
                memset(bucket,0,sizeof(*bucket));
                return 0;
            }
            if(!bucket->next && !bucket->prev){
                memset(bucket,0,sizeof(*bucket));
                ht->empty++;
                return 0;
            }
        }
    }else{
        if(tmp_e->prev && tmp_e->next){
            tmp_e->prev->next = tmp_e->next;
            tmp_e->next->prev = tmp_e->prev;
            free(tmp_e->key);free(tmp_e);
            return  0;
        }
        if(tmp_e->prev == NULL && tmp_e->next){
            tmp_l->entry = tmp_e->next;
            free(tmp_e->key);free(tmp_e);
            return 0;
        }
        if(tmp_e->prev && tmp_e->next == NULL){
            tmp_e->prev->next = NULL;
            free(tmp_e->key);free(tmp_e);
            return 0;
        }
    }
    return 666;
}

void hashtable_free(struct hashtable* ht)
{
    if(!ht)
        return;
    for(int i = 0; i < ht->bucket_amm; i++){
        struct ht_bucket* bucket = &ht->bucket[i];
        while(bucket){
            struct ht_llist_cont* tmp_c  = bucket->llist_cont;
            while(tmp_c){
                struct ht_llist_cont* n = tmp_c->next;
                struct ht_entry* tmp_e = tmp_c->llist->entry;
                while(tmp_e){
                    struct ht_entry* ne = tmp_e->next;
                    free(tmp_e->key);free(tmp_e);
                    tmp_e = ne;
                }
                free(tmp_c);
                tmp_c = n;
            }
            bucket->llist_cont = NULL;
            bucket = bucket->next;
        }
    }
    struct ht_llist* ll = ht->llist_head;
    while(ll){
        struct ht_llist* next = ll->next;
        free(ll);
        ll = next;
    }
    free(ht->bucket);
    free(ht);
}
int hashtable_iterate(struct hashtable* ht,void (*callback)(void* vptr))
{
    assert(ht); assert(callback);
    struct ht_llist* tmp_l = ht->llist_head;
    while(tmp_l){
        struct ht_entry* tmp_e = tmp_l->entry;
        while(tmp_e){
            callback(tmp_e->data);
            tmp_e = tmp_e->next;
        }
        tmp_l = tmp_l->next;
    }
    return 0;
}
int hashtable_iterate_wkey(struct hashtable* ht,void* usr,void (*callback)(char*,void* vptr,void* usr,size_t iter))
{
    assert(ht); assert(callback);
    struct ht_llist* tmp_l = ht->llist_head;
    size_t iter = 0;
    while(tmp_l){
        struct ht_entry* tmp_e = tmp_l->entry;
        while(tmp_e){
            callback(tmp_e->key,tmp_e->data,usr,iter);
            tmp_e = tmp_e->next;
            iter++;
        }
        tmp_l = tmp_l->next;
    }
    return 0;
}
