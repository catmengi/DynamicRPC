#pragma once
#include "rpccall.h"
#include <sys/types.h>
ssize_t rpcmsg_write_to_fd(struct rpcmsg* msg, int fd);
int get_rpcmsg_from_fd(struct rpcmsg* msg,int fd);
