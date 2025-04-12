default:
	mkdir build
	cc -Wall -c -g drpc_*.c impl_queue.c deque.c queue.c aes.c hashtable.c/*.c -O3
	mv *.o build/
	ar rcs drpc_full.a build/*.o
	rm -rf build

