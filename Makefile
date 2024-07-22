default:
	gcc -g  -o crpc-server testmain.c hashtable.c/hashtable.c rpcserver.c rpcbuff.c tqueque/tqueque.c rpcpack.c rpcmsg.c rpccall.c -lffi -lpthread -Wall #-fsanitize=address
	gcc -g  -o testc hashtable.c/hashtable.c rpcclient.c rpcbuff.c tqueque/tqueque.c rpcpack.c rpcmsg.c rpccall.c testclient.c -Wall #-fsanitize=address
