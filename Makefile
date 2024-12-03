default:
	gcc -o debug test.c drpc*.c hashtable.c/hashtable.c -g -lffi -pthread -Wall -Wpedantic -fsanitize=address -fsanitize=undefined --std=gnu23
	gcc -o debug32 test.c drpc*.c hashtable.c/hashtable.c -g -lffi -pthread -Wall -Wpedantic -fsanitize=address -fsanitize=undefined --std=gnu23 -m32


