default:
	gcc -o debug test.c drpc*.c hashtable.c/hashtable.c -g -lffi -pthread -Wall -Wpedantic -fsanitize=undefined
	gcc -o debug32 test.c drpc*.c hashtable.c/hashtable.c -g -lffi -pthread -Wall -Wpedantic -fsanitize=address -fsanitize=undefined -m32
	gcc -o debugS testS.c drpc*.c hashtable.c/hashtable.c -g -lffi -pthread -Wall -Wpedantic  -fsanitize=undefined 
	gcc -o debug32S testS.c drpc*.c hashtable.c/hashtable.c -g -lffi -pthread -Wall -Wpedantic -fsanitize=address -fsanitize=undefined -m32

