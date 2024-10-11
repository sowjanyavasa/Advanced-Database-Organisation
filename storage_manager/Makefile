CC = gcc

run_test_assign1_1: storage_mgr.o dberror.o test_assign1_1.o
	$(CC) storage_mgr.o dberror.o test_assign1_1.o -o run_test_assign1_1.exe

storage_mgr.o: storage_mgr.c storage_mgr.h
	$(CC) -c storage_mgr.c

dberror.o: dberror.c dberror.h
	$(CC) -c dberror.c

test_assign1_1.o: test_assign1_1.c
	$(CC) -c test_assign1_1.c

clean:
	rm -f *.o run_test_assign1_1.exe

run: run_test_assign1_1
	./run_test_assign1_1.exe