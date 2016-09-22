ll:malloc_spinlock_test

CC=g++
CPPFLAGS=-Wall -std=c++11 
LDFLAGS=-pthread

spinlock_test:malloc_spinlock_test.o
	$(CC) $(LDFLAGS) -o $@ $^

malloc_spinlock_test.o:malloc_spinlock_test.cpp
	$(CC) $(CPPFLAGS) -o $@ -c $^


.PHONY:
	clean

clean:
	rm malloc_spinlock_test; rm malloc_spinlock_test.o;
