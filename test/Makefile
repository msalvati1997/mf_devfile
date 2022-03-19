CC=gcc
CFLAGS=-lpthread

ALL:concurrency_test enable_disable_test timeout_test write_and_read_test

concurrency_test:
	$(CC) -o $@ concurrency_test.c $(CFLAGS)

enable_disable_test:
	$(CC) -o $@ enable_disable_test.c $(CFLAGS)

timeout_test:
	$(CC) -o $@ timeout_test.c $(CFLAGS)

write_and_read_test:
	$(CC) -o $@ write_and_read_test.c $(CFLAGS)

clean:
	rm -rf *.o concurrency_test enable_disable_test timeout_test write_and_read_test




