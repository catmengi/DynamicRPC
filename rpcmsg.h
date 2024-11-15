#pragma once
#include "rpccall.h"
#include <sys/types.h>
void cipher_xor(char* base, char* xor, uint8_t* out_buf, size_t out_buf_len);
int send_rpcmsg(struct rpcmsg* msg, int fd,uint8_t* aes128_key);
int get_rpcmsg(struct rpcmsg* msg,int fd,uint8_t* aes128_key);
