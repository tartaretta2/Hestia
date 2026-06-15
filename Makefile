CC = g++
CFLAGS = -Wall -I./include
SRC = src/*.cpp
TARGET = build/remote_ir

sim:
	mkdir -p build
	$(CC) $(CFLAGS) -DSIM $(SRC) -lpthread -o $(TARGET)

release:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -lpthread -lgpiod -o $(TARGET)

clean:
	rm -rf build