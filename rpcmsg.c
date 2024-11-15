#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include "rpcmsg.h"
#include "rpccall.h"
#include "lb_endian.h"
#include "aes.h"
#include <stdio.h>

void cipher_xor(uint8_t* base, char* xor, uint8_t* out_buf,size_t out_buf_len){
    memset(out_buf,0,out_buf_len);
    for(size_t i = 0; i < strlen(xor) && i <out_buf_len; i++){
        out_buf[i] = base[i] ^ xor[i];
    }
}

uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };


uint64_t nextDivisibleBy16(uint64_t value) {
    if (value % 16 == 0) {
        return value; // Already divisible by 16
    }
    return (value / 16 + 1) * 16; // Calculate the next multiple of 16
}

int send_rpcmsg(struct rpcmsg* msg, int fd,uint8_t* aes128_key){
    uint64_t encrypt_msg_len = nextDivisibleBy16(sizeof(char) + sizeof(uint64_t) + msg->payload_len);
    uint64_t full_msg_len = encrypt_msg_len + 8;
    char* fullmsg = calloc(full_msg_len,sizeof(char));
    assert(fullmsg);
    char* wr = fullmsg + sizeof(uint64_t);
    char* enc = wr;
    uint64_t be64_encrypt_msg_len = cpu_to_be64(encrypt_msg_len);
    memcpy(fullmsg,&be64_encrypt_msg_len,sizeof(uint64_t));

    *wr = msg->msg_type; wr++;
    if(msg->msg_type != CON && msg->msg_type != BAD && msg->msg_type != PING && msg->msg_type != DISCON){
        uint64_t be64_msg_payload_len = cpu_to_be64(msg->payload_len);
        memcpy(wr,&be64_msg_payload_len,sizeof(uint64_t)); wr += sizeof(uint64_t);
        if(msg->payload != NULL)
            memcpy(wr,msg->payload, msg->payload_len);
        free(msg->payload);
    }

    if(aes128_key){
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx,aes128_key,iv);
        AES_CBC_encrypt_buffer(&ctx,(uint8_t*)enc,encrypt_msg_len);
    }

    int ret = 0;
    if(send(fd,fullmsg,full_msg_len,MSG_NOSIGNAL) <= 0){ret = 1; goto exit;}
exit:
    memset(msg,0,sizeof(*msg));
    free(fullmsg);
    return ret;
}


int get_rpcmsg(struct rpcmsg* msg ,int fd,uint8_t* aes128_key){
    memset(msg,0,sizeof(*msg));
    int ret = 0;
    uint64_t encrypt_msg_len = 0;
    if(recv(fd,&encrypt_msg_len,sizeof(uint64_t),MSG_NOSIGNAL) <= 0) return 1;
    encrypt_msg_len = cpu_to_be64(encrypt_msg_len);
    char* enc = malloc(encrypt_msg_len);
    assert(encrypt_msg_len);
    if(recv(fd,enc,encrypt_msg_len,MSG_NOSIGNAL) <= 0) {free(enc); return 1;}

    if(aes128_key){
     struct AES_ctx ctx;
     AES_init_ctx_iv(&ctx,aes128_key,iv);
     AES_CBC_decrypt_buffer(&ctx,(uint8_t*)enc,encrypt_msg_len);
    }
    char* wr = enc;

    msg->msg_type = *wr; wr++;
    if(msg->msg_type != CON && msg->msg_type != BAD && msg->msg_type != PING && msg->msg_type != DISCON){
        uint64_t be64_msg_payload_len = 0;
        memcpy(&be64_msg_payload_len, wr, sizeof(uint64_t)); wr += sizeof(uint64_t);
        msg->payload_len = be64_to_cpu(be64_msg_payload_len);
        if(msg->payload_len > 0){
            msg->payload = malloc(msg->payload_len);
            memcpy(msg->payload, wr, msg->payload_len);
        }
    }
    free(enc);
    return 0;
}
