CC = g++
CFLAGS = -Wall -std=c++17 -I./include

LIBS = -lgpiod -llgpio -lpthread

#libraries for camera (OpenVINO, OpenCV, Tesseract)
CAMERA_LIBS = $(shell pkg-config --libs opencv4) \
              -ltesseract -lleptonica \
              -lopenvino
CAMERA_CFLAGS = $(shell pkg-config --cflags opencv4)

SRC = $(filter-out src/cameraYOLO.cpp, $(wildcard src/*.cpp))
CAMERA_SRC = src/cameraYOLO.cpp

TARGET = build/ir_ms_alarm

sim:
	mkdir -p build
	$(CC) $(CFLAGS) -DSIM $(SRC) -o $(TARGET)

release:
	mkdir -p build
	$(CC) $(CFLAGS) $(CAMERA_CFLAGS) $(SRC) $(CAMERA_SRC) -o $(TARGET) $(LIBS) $(CAMERA_LIBS)

clean:
	rm -rf build