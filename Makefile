CC = g++
CFLAGS = -Wall -std=c++17 -I./include
LIBS = -lgpiod -llgpio -lpthread

# Path OpenVINO (installato nella cartella dei pacchetti Python)
OPENVINO_DIR = /home/giovanni/.local/lib/python3.13/site-packages/openvino
OPENVINO_INC = $(OPENVINO_DIR)/include
OPENVINO_LIB = $(OPENVINO_DIR)/libs

# Librerie per la parte camera (OpenVINO + OpenCV + Tesseract)
CAMERA_CFLAGS = $(shell pkg-config --cflags opencv4) -I$(OPENVINO_INC)
CAMERA_LIBS = $(shell pkg-config --libs opencv4) \
              -ltesseract -lleptonica \
              -L$(OPENVINO_LIB) -Wl,-rpath,$(OPENVINO_LIB) -lopenvino

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