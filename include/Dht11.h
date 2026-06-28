#pragma once

#include <string>

using namespace std;

void initDHT11(const string& iio_path = "/sys/bus/iio/devices/iio:device0");

bool readDHT11(float& temp, float& hum);

void cleanupDHT11();