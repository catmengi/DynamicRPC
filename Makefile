default:
	mkdir build
	gcc -c -g drpc_*.c aes.c hashtable.c/*.c
	mv *.o build/
	ar rcs drpc_full.a build/*.o
	rm -rf build

