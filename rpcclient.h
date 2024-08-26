#pragma once
#include "rpccall.h"
struct rpccon{
   int fd;
   char* uniq;
   int stop;
   pthread_t ping;
   pthread_mutex_t send;
};
struct rpcclient_fninfo{
   enum rpctypes* proto;
   uint64_t protolen;
   char* name;
};
int rpcserver_connect(char* host,char* key,int portno,struct rpccon* con);
int rpcclient_call(struct rpccon* con,char* fn,enum rpctypes* rpctypes,char* flags, int rpctypes_len,void* fnret,...);
void rpcclient_discon(struct rpccon* con);
struct rpcclient_fninfo* rpcclient_list_functions(struct rpccon* con,uint64_t* fn_len);
void rpcclient_fninfo_free(struct rpcclient_fninfo* in,uint64_t len);
