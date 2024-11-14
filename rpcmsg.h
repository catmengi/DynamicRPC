#pragma once
#include "rpccall.h"
#include <sys/types.h>

int send_rpcmsg(struct rpcmsg* msg, int fd,uint8_t* aes128_key);
int get_rpcmsg(struct rpcmsg* msg,int fd,uint8_t* aes128_key);
