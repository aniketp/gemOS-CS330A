CC=	gcc
ASM=	as
CFLAGS= -lm

SRC=	main.c
PRM=	prime.s
OBJ=	prime.o

all:
	make clean
	make $(OBJ)
	make prime

prime.o: $(PRM)
	$(ASM) -o $@ -c $<

prime: $(SRC) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

run: prime
	./$<

.PHONY: clean

clean:
	rm -f prime prime.o
