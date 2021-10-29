.PHONY: all clean

CFLAGS := -Wall -Werror -O3 -g

all: basic_test

scapegoat_tree.o: ../scapegoat_tree.c ../scapegoat_tree.h
	gcc $(CFLAGS) -c -o scapegoat_tree.o ../scapegoat_tree.c

basic_test: basic_test.c scapegoat_tree.o
	gcc $(CFLAGS) -o basic_test basic_test.c scapegoat_tree.o

clean:
	rm -f *.o
	rm -f basic_test
