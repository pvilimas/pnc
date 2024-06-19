all: build run

build:
	gcc -std=gnu11 \
		pnc.c \
		number.c \
		runtime_functions.c \
		-o pnc -lm -lgmp

run:
	./pnc
