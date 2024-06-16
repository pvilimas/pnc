all: build run

build:
	gcc -std=gnu11 *.c -o pnc -lm

run:
	./pnc
