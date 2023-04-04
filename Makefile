CC=gcc
CFLAGS=-g -Wall -Wextra -std=c99

SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)
TARGETS=$(OBJS:%.o=%)

run_vma: build
	./vma

build: $(OBJS)
	$(CC) $(CFLAGS) -o vma $(OBJS)

pack:
	zip -FSr 312CA_DamianMihai_Robert_SD.zip README README.md Makefile *.c *.h

clean:
	rm -f $(TARGETS) $(OBJS)

.PHONY: pack clean
