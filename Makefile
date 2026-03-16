CC = g++
CFLAGS = -Wall -I./include
LIBS = -lgpiod -llgpio -lpthread
SRC = src/main.cpp src/motion_sensor.cpp src/led.cpp src/buzzer.cpp
TARGET = build/alarm

sim:
	mkdir -p build
	$(CC) $(CFLAGS) -DSIM $(SRC) -o $(TARGET)

release:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -rf build