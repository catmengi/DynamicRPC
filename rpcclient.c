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

struct rpccon{
   int fd;
   int perm;
   int stop;
   pthread_t ping;
   pthread_mutex_t send;
};

void* rpccon_keepalive(void* arg){
   struct rpccon* con = arg;
   while(!con->stop){
      pthread_mutex_lock(&con->send);
      struct rpcmsg msg = {PING,0,0,0};
      rpcmsg_write_to_fd(&msg,con->fd);
      pthread_mutex_unlock(&con->send);
      sleep(3);
   }
   pthread_detach(pthread_self());
   return NULL;
}

int rpcserver_connect(char* host,char* key,int portno,struct rpccon* con){
   if(!host || !key)
      return -1;
   int sockfd;
   struct sockaddr_in serv_addr;
   struct hostent *server;
   con->stop = 0;


   /* Create a socket point */
   sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sockfd <= 0) {
      close(sockfd);
      return -1;
   }

   server = gethostbyname(host);

   if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");close(sockfd);
      return -1;
   }
   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
   serv_addr.sin_port = htons(portno);
   /* Now connect to the server */
   if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0) {
      close(sockfd);
      return -1;
   }
   struct rpcmsg req = {0};
   struct rpcmsg ans = {0};
   req.msg_type = CON;
   if(rpcmsg_write_to_fd(&req,sockfd) == -1){
      close(sockfd);
      return 2;
   }
   struct rpctype auth = {0};
   uint64_to_type(_hash_fnc(key,strlen(key) + 1),&auth);
   req.msg_type = AUTH;
   req.payload = malloc((req.payload_len = type_buflen(&auth)));
   assert(req.payload);
   type_to_arr(req.payload,&auth);
   if(rpcmsg_write_to_fd(&req,sockfd) == -1){
      free(auth.data);
      free(req.payload);
      close(sockfd);
      return 2;
   }
   free(auth.data);
   free(req.payload);
   if(get_rpcmsg_from_fd(&ans,sockfd) != 0){
      close(sockfd);
      return 3;
   }
   if(ans.msg_type != OK) {
      free(ans.payload);
      close(sockfd);
      return 4;
   }
   arr_to_type(ans.payload,&auth);
   con->perm = type_to_int32(&auth);
   free(ans.payload);
   free(auth.data);
   con->fd = sockfd;
   pthread_mutex_init(&con->send,NULL);
   pthread_create(&con->ping,NULL,rpccon_keepalive,con);
   return 0;
}
int rpcclient_call(struct rpccon* con,char* fn,enum rpctypes* rpctypes,char* flags, int rpctypes_len,void* fnret,...){
   pthread_mutex_lock(&con->send);
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
   struct rpctype* args = calloc(rpctypes_len,sizeof(*args));
   assert(args);
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
         assert(flags);
         char flag = 0;
         if(flags != NULL) flag = flags[i];
         if(flag == 1) {resargs_upd[j] = buf; j++;}
         create_rpcbuff_type(buf,flag,&args[i]);
         continue;
      }
      if(rpctypes[i] == RPCSTRUCT){
         struct rpcstruct* buf = va_arg(vargs,struct rpcstruct*);
         assert(flags);
         char flag = 0;
         if(flags != NULL) flag = flags[i];
         if(flag == 1) {resargs_upd[j] = buf; j++;}
         create_rpcstruct_type(buf,flag,&args[i]);
         continue;
      }
   }
   struct rpccall call = {fn,rpctypes_len,args};
   struct rpcmsg req = {0};
   struct rpcmsg ans = {0};
   req.msg_type = CALL;
   req.payload = rpccall_to_buf(&call,&req.payload_len);
   if(rpcmsg_write_to_fd(&req,con->fd) < 0){
      rpctypes_free(args,rpctypes_len);
      free(req.payload);
      pthread_mutex_unlock(&con->send);
      return 1;
   }
   free(req.payload);
   rpctypes_free(args,rpctypes_len);
   if(get_rpcmsg_from_fd(&ans,con->fd) != 0){
      pthread_mutex_unlock(&con->send);
      return 2;
   };
   if(ans.msg_type != OK){
      pthread_mutex_unlock(&con->send);
      return ans.msg_type;
   }
   memset(&req,0,sizeof(req));
   req.msg_type = READY;
   rpcmsg_write_to_fd(&req,con->fd);
   get_rpcmsg_from_fd(&ans,con->fd);
   if(ans.msg_type != RET){
      if(ans.msg_type == DISCON){
          close(con->fd);
          con->stop = 1;
          pthread_mutex_unlock(&con->send);
          return DISCON;
      }else return ans.msg_type;
   }
   buf_to_rpcret(&ret,ans.payload);
   free(ans.payload);
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
         rpcstruct_free(resargs_upd[i]); //this is not heap-use-after-free since rpcstruct_free ONLY freeding struct internals
         memcpy(resargs_upd[i],new,sizeof(struct rpcstruct));
         free(new);
      }
   }
   enum rpctypes type = ret.ret.type;
   assert(fnret != NULL && type != VOID);
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
      return 0;
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
   free(ret.ret.data);
   rpctypes_free(ret.resargs,ret.resargs_amm);
   pthread_mutex_unlock(&con->send);
   return 0;
}
void rpcclient_discon(struct rpccon* con){
   con->stop = 1;
   struct rpcmsg msg = {DISCON,0,0,0};
   rpcmsg_write_to_fd(&msg,con->fd);
   close(con->fd);
}
