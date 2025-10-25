# Makefile to compile all the C files

CC = gcc
CFLAGS = -lpthread
TARGETS = sensor subsystem signal server UI

all: $(TARGETS)

sensor: sensor.c
	$(CC) sensor.c -o sensor $(CFLAGS)

subsystem: subsystem.c
	$(CC) subsystem.c -o subsystem $(CFLAGS)

signal: signal.c
	$(CC) signal.c -o signal

server: server.c
	$(CC) server.c -o server $(CFLAGS)

UI: UI.c
	$(CC) UI.c -o UI -lncursesw $(CFLAGS)

clean:
	rm -f $(TARGETS)
