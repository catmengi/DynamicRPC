#pragma once
#include <stdint.h>
#ifndef RPCTYPES_H
#define RPCTYPES_H
#include "rpctypes.h"
#endif
#define RPCCALL_H
struct rpccall{
    char* fn_name;
    uint64_t args_amm;
    struct rpctype* args;
};
struct rpcret{
    uint64_t resargs_amm;
    struct rpctype* resargs; /*args that was resended*/
    struct rpctype ret; /*libffi limitations support only one ret*/
};

enum rpcmsg_type{
    CON = 53, /*magic value*/
    CALL = 1,
    RET = 2,
    NOFN = 3,
    BAD = 4,
    OK = 5,
    AUTH = 6,
    DISCON = 7,
    LPERM = 8,
    PING = 9,
    LSFN = 10,
};
struct rpcmsg{
    enum rpcmsg_type msg_type;
    uint64_t payload_len;
    char* payload;
};

int is_rpctypes_equal(enum rpctypes* serv, uint64_t servlen, enum rpctypes* client, uint64_t clientlen);
uint64_t rpctypes_get_buflen(struct rpctype* rpctypes,uint64_t rpctypes_len);
void rpctypes_to_buf(struct rpctype* rpctypes,uint64_t rpctypes_amm, char* out);
struct rpctype* buf_to_rpctypes(char* in,uint64_t* rpctypes_amm);
enum rpctypes* rpctypes_get_types(struct rpctype* rpctypes,uint64_t rpctypes_amm);
void rpctypes_free(struct rpctype* rpctypes, uint64_t rpctypes_amm);
char* rpccall_to_buf(struct rpccall* rpccall, uint64_t* buflen);
void buf_to_rpccall(struct rpccall* rpccall,char* in);
struct rpctype* rpctypes_clean_nonres_args(struct rpctype* rpctypes, uint64_t rpctypes_len,uint64_t* retargsamm_out);
char* rpcret_to_buf(struct rpcret* rpcret, uint64_t* buflen);
void buf_to_rpcret(struct rpcret* ret,char* in);
