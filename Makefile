#Makefile for sim-eclipse.c
sim-eclipse: sim-eclipse.c
	gcc -Wall -O2 -pthread sim-eclipse.c -o simulator

clean:
	rm simulator
