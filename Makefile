default:
	gcc -o debug test.c drpc*.c hashtable.c/hashtable.c -g -lffi -pthread -Wall -Wpedantic -fsanitize=address -fsanitize=undefined

