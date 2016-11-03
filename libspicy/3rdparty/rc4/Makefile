CC = gcc
CFLAGS = -O2 -fomit-frame-pointer

all: rc4

rc4: rc4.c rc4.h
	$(CC) -Wall -W -ansi -pedantic $(CFLAGS) -DTEST_VECTORS $< -o $@

clean:
	- rm -f rc4
