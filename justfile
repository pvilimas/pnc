all: build run

build:
	gcc -std=gnu11 -Wall -Wextra \
		pnc.c \
		number.c \
		runtime_functions.c \
		-o pnc -lm -lgmp -lmpfr

run:
	./pnc
