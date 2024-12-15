default:
	gcc -o test test.c drpc*.c hashtable.c/hashtable.c aes.c -g -lffi -Wall -Wpedantic  -pthread -fsanitize=address

