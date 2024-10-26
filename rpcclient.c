#include "rpccall.h"
#include "hashtable.c/hashtable.h"
#include <pthread.h>
#include "rpcmsg.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "rpcpack.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "rpcclient.h"
#include "rpctypes.h"

void __rpcclient_disconnect_callback_initiate(struct rpcclient* self,
                                              enum rpcclient_disconnect_reason reason){
   if(self->disconnect_cb != NULL){
      self->disconnect_cb(self->disconnect_cb_user, reason);
   }

}

void* rpcclient_keepalive(void* arg){
   struct rpcclient* self = arg;
   while(self->stop == 0){
      pthread_mutex_lock(&self->send);
      struct rpcmsg msg = {PING,0,0};
      if(send_rpcmsg(&msg,self->fd) != 0 || get_rpcmsg(&msg,self->fd) != 0){
         self->stop = 1;
         close(self->fd);
         pthread_mutex_unlock(&self->send);
         __rpcclient_disconnect_callback_initiate(self,NET_FAILURE);
         return NULL;
      }
      pthread_mutex_unlock(&self->send);
      sleep(1);
   }
   return NULL;
}

struct rpcclient* rpcclient_connect(char* host,int portno,char* key){
   assert(host); assert(key);
   struct rpcclient* self = calloc(1,sizeof(struct rpcclient));
   assert(self);
   pthread_mutex_init(&self->send,NULL);
   int sockfd;
   struct sockaddr_in serv_addr;
   struct hostent *server;
   self->stop = 0;


   /* Create a socket point */
   sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sockfd <= 0) {
      close(sockfd);
      free(self);
      return NULL;
   }

   server = gethostbyname(host);

   if (server == NULL) {
      perror("gethostbyname");
      close(sockfd);
      free(self);
      return NULL;
   }
   memset(&serv_addr,0,sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   memcpy((char *)&serv_addr.sin_addr.s_addr,(char *)server->h_addr, server->h_length);
   serv_addr.sin_port = htons(portno);
   if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0) {
      perror("connect");
      close(sockfd);
      free(self);
      return NULL;
   }
   struct timeval time;
   time.tv_sec = 5;
   time.tv_usec = 0;
   assert(setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&time,sizeof(time)) == 0);
   assert(setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&time,sizeof(time)) == 0);
   struct rpcmsg reply = {0};
   struct rpcmsg gotmsg = {0};
   reply.msg_type = CON;
   if(send_rpcmsg(&reply,sockfd) != 0){
      close(sockfd);
      free(self);
      return NULL;
   }
   struct rpctype auth = {0};
   uint64_to_type(_hash_fnc(key,strlen(key) + 1),&auth);
   reply.msg_type = AUTH;
   reply.payload = malloc((reply.payload_len = type_buflen(&auth)));
   assert(reply.payload);
   type_to_arr(reply.payload,&auth);
   if(send_rpcmsg(&reply,sockfd) != 0){
      free(auth.data);
      close(sockfd);
      free(self);
      return NULL;
   }
   free(auth.data);
   if(get_rpcmsg(&gotmsg,sockfd) != 0){
      close(sockfd);
      free(self);
      return NULL;
   }
   if(gotmsg.msg_type != OK) {
      free(gotmsg.payload);
      close(sockfd);
      free(self);
      return NULL;
   }
   arr_to_type(gotmsg.payload,&auth);
   self->uniq = unpack_str_type(&auth);
   free(gotmsg.payload);
   self->fd = sockfd;
   pthread_create(&self->ping,NULL,rpcclient_keepalive,self);
   return self;
}
int rpcclient_call(struct rpcclient* self,char* fn,enum rpctypes* rpctypes,char* flags, int rpctypes_len,void* fnret,...){
   if(self == NULL) return 1;
   if(self->stop == 1) return 1;
   pthread_mutex_lock(&self->send);
   va_list vargs;
   void** resargs_upd = NULL;
   uint8_t resargs_updl = 0;
   struct rpcret ret = {0};
   if(flags)
      for(uint8_t i = 0; i < rpctypes_len; i++)
         if(flags[i] == 1)
            resargs_updl++;
   if(resargs_updl != 0){
      resargs_upd = calloc(resargs_updl,sizeof(void*));
      assert(resargs_upd);
   }
   va_start(vargs, fnret);
   struct rpctype* args = NULL;
   if(rpctypes_len > 0){
      args = calloc(rpctypes_len,sizeof(*args));
      assert(args);
   }
   uint8_t j = 0;
   for(uint8_t i = 0; i < rpctypes_len; i++){
      if(rpctypes[i] == CHAR){
         char ch = va_arg(vargs,int);
         char_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == UINT16){
         uint16_t ch = va_arg(vargs,int);
         uint16_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == INT16){
         int16_t ch = va_arg(vargs,int);
         int16_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == UINT32){
         uint32_t ch = va_arg(vargs,uint32_t);
         uint32_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == INT32){
         int32_t ch = va_arg(vargs,int32_t);
         int32_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == UINT64){
         uint64_t ch = va_arg(vargs,uint64_t);
         uint64_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == INT64){
         int64_t ch = va_arg(vargs,int64_t);
         int64_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == FLOAT){
         double ch = va_arg(vargs,double);
         float_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == DOUBLE){
         double ch = va_arg(vargs,double);
         double_to_type(ch,&args[i]);
         continue;
      }
      if(rpctypes[i] == STR){
         char* ch = va_arg(vargs,char*);
         char flag = 0;
         if(flags != NULL) flag = flags[i];
         if(flag == 1) {resargs_upd[j] = ch; j++;}
         create_str_type(ch,flag,&args[i]);
         continue;
      }
      if(rpctypes[i] == SIZEDBUF){
         char* ch = va_arg(vargs,char*);
         char flag = 0;
         if(flags != NULL) flag = flags[i];
         if(flag == 1) {resargs_upd[j] = ch; j++;}
         uint64_t buflen = va_arg(vargs,unsigned int);
         create_sizedbuf_type(ch,buflen,flag,&args[i]);
         continue;
      }
      if(rpctypes[i] == RPCBUFF){
         struct rpcbuff* buf = va_arg(vargs,struct rpcbuff*);
         char flag = 0;
         if(flags != NULL) flag = flags[i];
         if(flag == 1) {resargs_upd[j] = buf; j++;}
         create_rpcbuff_type(buf,flag,&args[i]);
         continue;
      }
      if(rpctypes[i] == RPCSTRUCT){
         struct rpcstruct* buf = va_arg(vargs,struct rpcstruct*);
         char flag = 0;
         if(flags != NULL) flag = flags[i];
         if(flag == 1) {resargs_upd[j] = buf; j++;}
         create_rpcstruct_type(buf,flag,&args[i]);
         continue;
      }
   }
   struct rpccall call = {fn,rpctypes_len,args};
   struct rpcmsg reply = {0};
   struct rpcmsg gotmsg = {0};
   reply.msg_type = CALL;
   reply.payload = rpccall_to_buf(&call,&reply.payload_len);
   if(send_rpcmsg(&reply,self->fd) != 0){
      rpctypes_free(args,rpctypes_len);
      close(self->fd);
      self->stop = 1;
      pthread_mutex_unlock(&self->send);
      __rpcclient_disconnect_callback_initiate(self,NET_FAILURE);
      return 10;
   }
   rpctypes_free(args,rpctypes_len);
   if(get_rpcmsg(&gotmsg,self->fd) != 0){
      close(self->fd);
      self->stop = 1;
      pthread_mutex_unlock(&self->send);
      __rpcclient_disconnect_callback_initiate(self,NET_FAILURE);
      return 20;
   }
   if(gotmsg.msg_type != RET){
      pthread_mutex_unlock(&self->send);
      return gotmsg.msg_type;
   }
   pthread_mutex_unlock(&self->send);
   assert(buf_to_rpcret(&ret,gotmsg.payload,gotmsg.payload_len) == 0);
   free(gotmsg.payload);
   assert(resargs_updl == ret.resargs_amm);
   for(uint8_t i = 0; i < ret.resargs_amm; i++){
      if(ret.resargs[i].type == STR){
         char* new = unpack_str_type(&ret.resargs[i]);
         assert(new);
         strcpy(resargs_upd[i],new);
      }
      if(ret.resargs[i].type == SIZEDBUF){
         uint64_t cpylen = 0;
         char* new = unpack_sizedbuf_type(&ret.resargs[i],&cpylen);
         assert(new);
         memcpy(resargs_upd[i],new,cpylen);
      }
      if(ret.resargs[i].type == RPCBUFF){
         struct rpcbuff* new = unpack_rpcbuff_type(&ret.resargs[i]);
         assert(new);
         __rpcbuff_free_N_F_C(resargs_upd[i]);
         memcpy(resargs_upd[i],new,sizeof(struct rpcbuff));
         free(new);
      }
      if(ret.resargs[i].type == RPCSTRUCT){
         struct rpcstruct* new = unpack_rpcstruct_type(&ret.resargs[i]);
         __rpcstruct_free(resargs_upd[i]); //this is not heap-use-after-free since rpcstruct_free ONLY freeding struct internals
         memcpy(resargs_upd[i],new,sizeof(struct rpcstruct));
         free(new);
      }
   }
   enum rpctypes type = ret.ret.type;
   assert((fnret == NULL && type == VOID) || (fnret != NULL && type != VOID) || (fnret != NULL && type == VOID));
   if(type == CHAR){
      char ch = type_to_char(&ret.ret);
      *(char*)fnret = ch;
   }
   if(type == UINT16){
      uint16_t ch = type_to_uint16(&ret.ret);
      *(uint16_t*)fnret = ch;
   }
   if(type == INT16){
      int32_t ch = type_to_int32(&ret.ret);
      *(int32_t*)fnret = ch;
   }
   if(type == UINT32){
      uint32_t ch = type_to_uint32(&ret.ret);
      *(uint32_t*)fnret = ch;
   }
   if(type == INT32){
      int32_t ch = type_to_int32(&ret.ret);
      *(int32_t*)fnret = ch;
   }
   if(type == UINT64){
       uint64_t ch = type_to_uint64(&ret.ret);
      *(uint64_t*)fnret = ch;
   }
   if(type == INT64){
      int64_t ch = type_to_int64(&ret.ret);
      *(int64_t*)fnret = ch;
   }
   if(type == FLOAT){
      float ch = type_to_float(&ret.ret);
      *(float*)fnret = ch;
    }
   if(type == DOUBLE){
      double ch = type_to_double(&ret.ret);
      *(double*)fnret = ch;
    }
    if(type == STR){
      char* ch = unpack_str_type(&ret.ret);
      *(char**)fnret = ch;
    }
    if(type == RPCBUFF){
      struct rpcbuff* ch = unpack_rpcbuff_type(&ret.ret);
      *(struct rpcbuff**)fnret = ch;
    }
    if(type == RPCSTRUCT){
      struct rpcstruct* ch = unpack_rpcstruct_type(&ret.ret);
      *(struct rpcstruct**)fnret = ch;
    }
   free(resargs_upd);
   if(type != STR) free(ret.ret.data);
   rpctypes_free(ret.resargs,ret.resargs_amm);
   return 0;
}
struct rpcclient_fninfo* rpcclient_list_functions(struct rpcclient* self,uint64_t* fn_len){
   if(self == NULL) return NULL;
   if(self->stop == 1) return NULL;
   pthread_mutex_lock(&self->send);
   struct rpcmsg reply = {LSFN,0,0};
   struct rpcmsg gotmsg = {0};
   if(send_rpcmsg(&reply,self->fd) != 0){
      pthread_mutex_unlock(&self->send);
      close(self->fd);
      __rpcclient_disconnect_callback_initiate(self,NET_FAILURE);
   }
   if(get_rpcmsg(&gotmsg,self->fd) != 0){
      pthread_mutex_unlock(&self->send);
      close(self->fd);
      __rpcclient_disconnect_callback_initiate(self,NET_FAILURE);
   }
   if(gotmsg.msg_type != LSFN){
      pthread_mutex_unlock(&self->send);
      return NULL;
   }
   struct rpcstruct* lsfn;
   struct rpctype otype;
   arr_to_type(gotmsg.payload,&otype);
   free(gotmsg.payload);
   assert((lsfn = unpack_rpcstruct_type(&otype)) != NULL);
   free(otype.data);
   char** fns = rpcstruct_get_fields(lsfn,fn_len);
   struct rpcclient_fninfo* fns_info = calloc(*fn_len,sizeof(struct rpcclient_fninfo));
   assert(fns_info);
   for(uint64_t i = 0; i < *fn_len; i++){
      fns_info[i].name = malloc(strlen(fns[i]) + 1);
      assert(fns_info[i].name);
      strcpy(fns_info[i].name,fns[i]);

      char* out_proto = NULL;
      uint64_t len = 0;
      assert(rpcstruct_get(lsfn,fns[i],SIZEDBUF,&out_proto,&len) == 0);
      assert(out_proto);
      fns_info[i].proto = calloc(len,sizeof(enum rpctypes));
      fns_info[i].protolen = len;
      assert(fns_info[i].proto);
      for(uint64_t j = 0; j < len; j++)
         fns_info[i].proto[j] = out_proto[j];
      free(out_proto);
   }
   free(fns);
   rpcstruct_free(lsfn); free(lsfn);
   pthread_mutex_unlock(&self->send);
   return fns_info;
}
void rpcclient_fninfo_free(struct rpcclient_fninfo* in,uint64_t len){
   for(uint64_t i = 0; i < len; i++){
      free(in[i].proto);
      free(in[i].name);
   }
   free(in);
}
void rpcclient_disconnect(struct rpcclient* self){
   if(self == NULL) return;
   if(self->stop == 1) return;
   self->stop = 1;
   pthread_mutex_lock(&self->send);
   struct rpcmsg msg = {DISCON,0,0};
   send_rpcmsg(&msg,self->fd);
   close(self->fd);
   if(self->disconnect_cb != NULL){
      self->disconnect_cb(self->disconnect_cb_user,INITIATED);
   }
   free(self->uniq);
   pthread_mutex_unlock(&self->send);
   pthread_join(self->ping,NULL);
   free(self);
}
void rpcclient_register_disconnect_cb(struct rpcclient* self, rpcclient_disconnect_cb cb, void* user){
   assert(self);
   self->disconnect_cb_user = user;
   self->disconnect_cb = cb;
}
