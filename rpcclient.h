#pragma once
#include "rpccall.h"
struct rpccon{
   int fd;
   int perm;
   int stop;
   pthread_t ping;
   pthread_mutex_t send;
};

int rpcserver_connect(char* host,char* key,int portno,struct rpccon* con);
int rpcclient_call(struct rpccon* con,char* fn,enum rpctypes* rpctypes,char* flags, int rpctypes_len,void* fnret,...);
void rpcclient_discon(struct rpccon* con);
