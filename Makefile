CC = gcc
CFLAGS = -Wall -O2 -g
TPL = TPL/tpl.c
SRC = main.c $(TPL)
TARGET = serialize_demo

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

