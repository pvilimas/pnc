all: build run

build:
	gcc -std=gnu11 -Wall -Wextra \
		src/pnc.c \
		src/number.c \
		src/runtime_functions.c \
		-o build/pnc -lm -lgmp -lmpfr

run:
	./build/pnc
