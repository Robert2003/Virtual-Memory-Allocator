# Copyright Damian Mihai-Robert 312CAb 2022-2023

CC=gcc
CFLAGS=-g -Wall -Wextra -std=c99

SRCS=$(wildcard src/VMA/*.c) $(wildcard src/helpers/*.c) $(wildcard src/*.c)
OBJS=$(SRCS:%.c=%.o)
TARGETS=$(OBJS:%.o=%)

run_vma: build
	./vma

build: $(OBJS)
	$(CC) $(CFLAGS) -o vma $(OBJS)

pack:
	zip -FSr 312CA_DamianMihai-Robert_SD.zip README Makefile src/helpers/*.c src/helpers/*.h src/VMA/*.c src/VMA/*.h

clean:
	rm -f $(TARGETS) $(OBJS)

.PHONY: pack clean
