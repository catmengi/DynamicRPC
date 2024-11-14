#pragma once
#include "rpccall.h"
#include <stdint.h>
enum rpcclient_disconnect_reason{
   INITIATED = 100,
   NET_FAILURE = 200,
};

typedef void (*rpcclient_disconnect_cb)(void* user, enum rpcclient_disconnect_reason reason);

struct rpcclient{
   int fd;
   char* fingerprint;
   int stop;
   pthread_t ping;
   pthread_mutex_t send;
   void* disconnect_cb_user;
   char cipher[16];
   rpcclient_disconnect_cb disconnect_cb;
};
struct rpcclient_fninfo{
   enum rpctypes* proto;
   uint64_t protolen;
   char* name;
};


struct rpcclient* rpcclient_connect(char* host,int portno,char* key);
int rpcclient_call(struct rpcclient* self,char* fn,enum rpctypes* fn_prototype,char* flags, uint8_t fn_prototype_len,void* fnret,...);
void rpcclient_disconnect(struct rpcclient* self);
struct rpcclient_fninfo* rpcclient_list_functions(struct rpcclient* self,size_t* fn_len);
void rpcclient_fninfo_free(struct rpcclient_fninfo* in,size_t len);
void rpcclient_register_disconnect_cb(struct rpcclient* self, rpcclient_disconnect_cb cb, void* user);
char* rpcclient_get_fingerprint(struct rpcclient* self);       //gets client uniq fingerprint, same as FINGERPRINT type on server
