default:
	mkdir build
	cc -c -g drpc_*.c queue.c aes.c hashtable.c/*.c -O3
	mv *.o build/
	ar rcs drpc_full.a build/*.o
	rm -rf build

