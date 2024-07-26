#include <stdint.h>
#ifndef RPCTYPES_H
#define RPCTYPES_H
#include "rpctypes.h"
#endif
#define RPCCALL_H
struct rpccall{
    char* fn_name;
    uint8_t args_amm;
    struct rpctype* args;
};
struct rpcret{
    uint8_t resargs_amm;
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
    READY = 6,
    AUTH = 7,
    NONREADY = 8,
    DISCON = 9,
    LPERM = 10,
    //LSFN = 11
};
struct rpcmsg{
    enum rpcmsg_type msg_type;
    uint64_t payload_len;
    char* payload;
    uint8_t payload_crc;
};
// void rpctypes_free(struct rpctype* rpctypes);
// enum rpctypes* ffi_types_to_rpctypes(ffi_type** ffi_types, size_t ffi_types_amm);
// ffi_type** rpctypes_to_ffi_types(enum rpctypes* rpctypes,size_t rpctypes_amm);
// char* rpccall_to_buf(struct rpccall* rpccall, uint64_t* buflen);
// enum rpctypes* rpctypes_get_types(struct rpctype* rpctypes,size_t* len);
// struct rpctype*  buf_to_rpctypes(char* in);
// int rpctypes_to_buf(struct rpctype* rpctypes, char* out);
// size_t rpctypes_get_buflen(struct rpctype* rpctypes);
// struct rpctype*  rpctypes_clean_nonres_args(struct rpctype* rpctypes);
// char* rpcret_to_buf(struct rpcret* rpcret, uint64_t* buflen);
// void buf_to_rpcret(struct rpcret* ret,char* in);
// int is_rpctypes_equal(enum rpctypes* frst, size_t frstlen, enum rpctypes* scnd, size_t scndlen);
// int buf_to_rpccall(struct rpccall* call,char* in);
int is_rpctypes_equal(enum rpctypes* frst, size_t frstlen, enum rpctypes* scnd, uint8_t scndlen);
size_t rpctypes_get_buflen(struct rpctype* rpctypes,uint8_t rpctypes_len);
int rpctypes_to_buf(struct rpctype* rpctypes,uint8_t rpctypes_amm, char* out);
struct rpctype* buf_to_rpctypes(char* in,uint8_t* rpctypes_amm);
enum rpctypes* rpctypes_get_types(struct rpctype* rpctypes,uint8_t rpctypes_amm);
void rpctypes_free(struct rpctype* rpctypes, uint8_t rpctypes_amm);
char* rpccall_to_buf(struct rpccall* rpccall, uint64_t* buflen);
int buf_to_rpccall(struct rpccall* rpccall,char* in);
struct rpctype* rpctypes_clean_nonres_args(struct rpctype* rpctypes, uint8_t rpctypes_len,uint8_t* retargsamm_out);
char* rpcret_to_buf(struct rpcret* rpcret, uint64_t* buflen);
void buf_to_rpcret(struct rpcret* ret,char* in);
