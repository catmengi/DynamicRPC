default:
	gcc -o debug test.c drpc*.c hashtable.c/hashtable.c aes.c -g -lffi -pthread
	gcc -o debugS testS.c drpc*.c hashtable.c/hashtable.c aes.c -g -lffi -pthread

