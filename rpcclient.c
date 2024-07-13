#include "rpccall.h"
#include "rpcmsg.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "rpcpack.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(){
   int sockfd, portno, n;
   struct sockaddr_in serv_addr;
   struct hostent *server;

   char buffer[256];


   portno = 2077;

   /* Create a socket point */
   sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

   if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }

   server = gethostbyname("localhost");

   if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
   }

   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
   serv_addr.sin_port = htons(portno);

   /* Now connect to the server */
   if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR connecting");
      exit(1);
   }
   struct rpctype type;
   create_sizedbuf_type(strdup("hello da fuck"),strlen("hello da fuck") + 1,1,&type);
   char* typer = malloc(type_buflen(&type)); type_to_arr(typer,&type);
   struct rpcmsg msg = {CON, 0,NULL,0};
   rpcmsg_write_to_fd(&msg,sockfd);
   struct rpcmsg auth = {AUTH,type_buflen(&type),typer};
   rpcmsg_write_to_fd(&auth,sockfd);
   get_rpcmsg_from_fd(&msg,sockfd);
   if(msg.msg_type != OK) {printf("failed the auth\n"); return 0;}
   for(;;){
      msg.msg_type = CALL;
      size_t index[1] = {52};
      struct rpcbuff* buf;
      struct rpctype* ll = calloc(4,sizeof(*ll)); create_rpcbuff_type(buf = rpcbuff_create(index,sizeof(index) / sizeof(index[0]),1),0,&ll[0]);
      create_sizedbuf_type("hello fda world",sizeof("hello fda world"),0,&ll[1]);
      uint64_to_type(525252,&ll[2]);
      create_str_type("42 братуха",0,&ll[3]);
      _rpcbuff_free(buf);
      struct rpccall call = {"buft",4,ll};
      msg.payload = rpccall_to_buf(&call,&msg.payload_len);
      struct rpcmsg got = {0};
      rpcmsg_write_to_fd(&msg,sockfd);
      rpctypes_free(call.args,call.args_amm);
      free(msg.payload);
      get_rpcmsg_from_fd(&got,sockfd);
      if(got.msg_type == OK){
         msg.msg_type = READY;
         rpcmsg_write_to_fd(&msg,sockfd);
      }else if(got.msg_type == NOFN || got.msg_type == LPERM || got.msg_type == BAD){
         printf("bad thing happened\n");
         msg.msg_type = DISCON;
         rpcmsg_write_to_fd(&msg,sockfd);
         return 0;
      }
      got.payload = NULL;
      get_rpcmsg_from_fd(&got,sockfd);
      if(got.payload == NULL) {puts("why");printf("%d\n",got.msg_type);close(sockfd);return 1;}
      struct rpcret ret = {0};buf_to_rpcret(&ret,got.payload);
      struct rpcbuff* buf2 = unpack_rpcbuff_type(&ret.ret);
      index[0] = 0;
      for(;index[0] < buf2->dimsizes[0]; index[0]++){
        // printf("%lu:%s\n",index[0],rpcbuff_getlast_from(buf2,index,sizeof(index) / sizeof(index[0]),NULL));
      }
      printf("%p: ret->resargs\n",ret.resargs);
      _rpcbuff_free(buf2);
      free(ret.ret.data);
      rpctypes_free(ret.resargs,ret.resargs_amm);
      puts("\n\n\n\n calling next time \n\n\n\n");
}
      msg.msg_type = DISCON;
      msg.payload = NULL;
      msg.payload_len = 0;
      rpcmsg_write_to_fd(&msg,sockfd);
   close(sockfd);
}
