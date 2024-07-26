#include <stdint.h>
#include <sys/types.h>
enum rpctypes{
    VOID = 0,
    CHAR = 1,
    STR = 2,
    UINT16 = 3,
    INT16 = 4,
    UINT32 = 5,
    INT32 = 6,
    UINT64 = 7,
    INT64 = 8,
    FLOAT = 9,
    DOUBLE = 10,
    RPCBUFF = 11,
    SIZEDBUF = 12, //1D array that will pass uint64_t typed value as his len in next arg: "void*, uint64_t"
    PSTORAGE = 13,
    INTERFUNC = 14,



    SMELLOFSHIT //type is used to determine bad data
};

struct __rpcbuff_el{
        struct __rpcbuff_el* childs;
        uint64_t elen;
        char* endpoint;
};
struct rpcbuff{
    struct __rpcbuff_el* start;
    uint64_t* dimsizes;
    uint64_t dimsizes_len;
    uint64_t lastdim_len;
};
struct rpctype{
  char type;
  char flag;
  uint64_t datalen;
  char* data;
};
struct rpcbuff* rpcbuff_create(uint64_t* dimsizes,uint64_t dimsizes_len,uint64_t lastdim_len);  /*lastdim_len set default last dimension len(rpcbuff_getlast_from create this size lastdim if
                                                                                          this  endpoint wasnt writen by rpcbuff_pushto), but rpcbuff_pushto will fit bigger data*/
void _rpcbuff_free(struct rpcbuff* rpcbuff);
char* rpcbuff_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len,uint64_t* outlen);
int rpcbuff_pushto(struct rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len, char* data, uint64_t data_len);
char* rpcbuff_to_arr(struct rpcbuff* rpcbuff,uint64_t* buflen);
struct rpcbuff* buf_to_rpcbuff(char* buf);
struct __rpcbuff_el* __rpcbuff_el_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len);
