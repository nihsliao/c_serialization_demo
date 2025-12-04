CC = gcc
CXX = g++
CFLAGS = -Wall -O2 -g #-fsanitize=address -fno-omit-frame-pointer
CXXFLAGS=-Wall -O2 -g -std=c++17 -pthread

C_LIBS=TPL/tpl.c MPACK/mpack/*.c NANOPB/nanopb/*.c
C_LIBS_TARGET = c_libs.a

CLIBSFLAGS += -ITPL
CLIBSFLAGS += -IMPACK/mpack -D MPACK_STDLIB=0
CLIBSFLAGS += -INANOPB/nanopb
CFLAGS_QUEUE = -Iconcurrentqueue

SRC = main.c $(TPL) $(MPACK) $(NANOPB)
TARGET = serialize_demo

SRC_SOCKET = main_socket.cpp $(TPL) $(MPACK) $(NANOPB)
TARGET_SOCKET := $(TARGET)_socket

LDLIBS += -lm

all: $(TARGET) $(TARGET_SOCKET)

$(C_LIBS_TARGET): $(C_LIBS)
	$(CC) $(CFLAGS) $(CLIBSFLAGS) -c $(C_LIBS)
	ar rcs $(C_LIBS_TARGET) *.o

$(TARGET): $(C_LIBS_TARGET) $(SRC)
	$(CC) $(CFLAGS) $(CLIBSFLAGS) -o $@ $(SRC) $(C_LIBS_TARGET) $(LDLIBS)

$(TARGET_SOCKET): $(C_LIBS_TARGET) $(SRC_SOCKET)
	$(CXX) $(CXXFLAGS) $(CFLAGS_QUEUE) $(CLIBSFLAGS) -o $@ $(SRC_SOCKET) $(C_LIBS_TARGET) $(LDLIBS)


clean:
	rm -f *.o $(C_LIBS_TARGET) $(TARGET) $(TARGET_SOCKET)
