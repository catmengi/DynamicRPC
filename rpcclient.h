#pragma once
#include "rpccall.h"

enum rpcclient_disconnect_reason{
   INITIATED,
   NET_FAILURE,
};

typedef void (*rpcclient_disconnect_cb)(void* user, enum rpcclient_disconnect_reason reason);

struct rpcclient{
   int fd;
   char* uniq;
   int stop;
   pthread_t ping;
   pthread_mutex_t send;
   void* disconnect_cb_user;
   rpcclient_disconnect_cb disconnect_cb;
};
struct rpcclient_fninfo{
   enum rpctypes* proto;
   uint64_t protolen;
   char* name;
};


struct rpcclient* rpcclient_connect(char* host,int portno,char* key);
int rpcclient_call(struct rpcclient* self,char* fn,enum rpctypes* rpctypes,char* flags, int rpctypes_len,void* fnret,...);
void rpcclient_disconnect(struct rpcclient* self);
struct rpcclient_fninfo* rpcclient_list_functions(struct rpcclient* self,uint64_t* fn_len);
void rpcclient_fninfo_free(struct rpcclient_fninfo* in,uint64_t len);
void rpcclient_register_disconnect_cb(struct rpcclient* self, rpcclient_disconnect_cb cb, void* user);
