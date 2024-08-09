#pragma once
#define ALLOC_ERR 2
#define ARG_ERR 1
#define ENOTFOUND 4
#define ENOSPACE 3
#include <sys/types.h>
#include <stdint.h>
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
uint64_t _hash_fnc(char* str,uint32_t keylen);
