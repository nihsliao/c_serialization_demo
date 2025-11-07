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

LDLIBS += -lm

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

clean:
	rm -f $(TARGET)

