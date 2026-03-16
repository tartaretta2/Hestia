CC = g++
CFLAGS = -Wall -I./include
LIBS = -lgpiod -llgpio -lpthread
SRC = src/*.cpp
TARGET = build/alarm

sim:
	mkdir -p build
	$(CC) $(CFLAGS) -DSIM $(SRC) -o $(TARGET)

release:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -rf build