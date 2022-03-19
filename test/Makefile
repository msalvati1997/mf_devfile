CC=gcc
CFLAGS=-lpthread

ALL:a1 a2 a3

concurrency_test:
	$(CC) -o $@ concurrency_test.c $(CFLAGS)

enable_disable_test:
	$(CC) -o $@ enable_disable_test.c $(CFLAGS)

timeout_test:
	$(CC) -o $@ timeout_test.c $(CFLAGS)

write_and_read_test:
	$(CC) -o $@ write_and_read_test.c $(CFLAGS)

clean:
	rm -rf *.o a1 a2 a3




