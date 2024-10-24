#pragma once
#include "rpccall.h"
#include <sys/types.h>
int send_rpcmsg(struct rpcmsg* msg, int fd);
int get_rpcmsg(struct rpcmsg* msg,int fd);
