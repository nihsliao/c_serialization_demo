CC = gcc
CFLAGS = -Wall -O2 -g
TPL = TPL/tpl.c
MPACK = MPACK/mpack/*
SRC = main.c $(TPL) $(MPACK)
TARGET = serialize_demo

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

