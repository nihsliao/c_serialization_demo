CC = gcc
CFLAGS = -Wall -O2 -g

TPL = TPL/tpl.c
CFLAGS += -ITPL

MPACK = MPACK/mpack/*.c
CFLAGS += -IMPACK/mpack

NANOPB = NANOPB/nanopb/*.c
CFLAGS += -INANOPB/nanopb

SRC = main.c $(TPL) $(MPACK) $(NANOPB)
TARGET = serialize_demo

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

