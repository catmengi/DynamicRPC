#pragma once
#define ALLOC_ERR 2
#define ARG_ERR 1
#define ENOTFOUND 4
#define ENOSPACE 3
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>


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

typedef struct hashtable* hashtable_t;


int hashtable_create(hashtable_t* ht,size_t size,size_t coll_pbuck);                             /*create new hashtable with size and collisions per bucket and pass it to hashtable_t pointer*/         


int hashtable_add(hashtable_t ht,char* key,uint32_t keylen,void* vptr,uint64_t meta);            /*add new entry to hashtable*/


int hashtable_rehash(hashtable_t ht,uint64_t size);                                              /*change size of hashtable*/


int hashtable_get(hashtable_t ht, char* key, uint32_t keylen, void** out);                      //
                                                                                                //   get data/key from entry and pass pointer to it into out;
                                                                                                //   return error code
                                                                                                //
int hashtable_getwmeta(hashtable_t ht, char* key, uint32_t keylen, void** out,uint64_t* meta);
int hashtable_get_key(hashtable_t ht, char* key, uint32_t keylen, char** out);                                      

int hashtable_remove_entry(hashtable_t ht,char* key,uint32_t keylen);                                                       /*remove entry from hashtable dont affect data*/

void hashtable_free(hashtable_t ht);                                                                                        /*remove hashtable and free all keys, dont affect data*/

int hashtable_iterate(struct hashtable* ht,void (*callback)(void* vptr));
int hashtable_iterate_wkey(struct hashtable* ht,void* usr,void (*callback)(char*,void* vptr,void* usr,size_t iter));
int hashtable_get_by_hash(struct hashtable* ht, uint64_t hash, void** out);
int hashtable_get_key_by_hash(struct hashtable* ht, uint64_t hash, char** out);
uint64_t _hash_fnc(char* str,uint32_t keylen);
