#include "rpccall.h"
#include "hashtable.c/hashtable.h"
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
};


int rpcserver_connect(char* host,char* key,int portno,struct rpccon* con){
   if(!host || !key)
      return -1;
   int sockfd;
   struct sockaddr_in serv_addr;
   struct hostent *server;


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
   return 0;
}
int rpcclient_call(struct rpccon* con,char* fn,enum rpctypes* rpctypes,char* flags, int rpctypes_len,struct rpcret* fnret,...){
   va_list vargs;
   void* tmp; //tmp variable for storing va_arg;
   va_start(vargs, fnret);
   struct rpctype* args = calloc(rpctypes_len,sizeof(*args));
   assert(args);
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
         assert(flags);
         create_str_type(ch,flags[i],&args[i]);
         continue;
      }
      if(rpctypes[i] == SIZEDBUF){
         char* ch = va_arg(vargs,char*);
         assert(flags);
         uint64_t buflen = va_arg(vargs,uint64_t);
         create_sizedbuf_type(ch,buflen,flags[i],&args[i]);
         continue;
      }
      if(rpctypes[i] == RPCBUFF){
         struct rpcbuff* buf = va_arg(vargs,struct rpcbuff*);
         assert(flags);
         create_rpcbuff_type(buf,flags[i],&args[i]);
         continue;
      }
      if(rpctypes[i] == RPCSTRUCT){
         struct rpcstruct* buf = va_arg(vargs,struct rpcstruct*);
         assert(flags);
         create_rpcstruct_type(buf,flags[i],&args[i]);
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
      return 1;
   }
   free(req.payload);
   rpctypes_free(args,rpctypes_len);
   if(get_rpcmsg_from_fd(&ans,con->fd) != 0){
      return 2;
   };
   if(ans.msg_type != OK){
      return ans.msg_type;
   }
   memset(&req,0,sizeof(req));
   req.msg_type = READY;
   rpcmsg_write_to_fd(&req,con->fd);
   get_rpcmsg_from_fd(&ans,con->fd);
   if(ans.msg_type != RET){
      if(ans.msg_type == DISCON){
          close(con->fd);
          return DISCON;
      }else return ans.msg_type;
   }
   buf_to_rpcret(fnret,ans.payload);
   free(ans.payload);
   return 0;
}
