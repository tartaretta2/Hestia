#include "Dht11.h"
#include <fstream>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace std;

static string temp_file;
static string hum_file;
static bool initialized = false;

// Reads a single value from a kernel iio system file
static bool readSensorFile(const string& path, float& value)
{
    ifstream f(path);
    if (!f.is_open())
    {
        return false;
    }
    
    long raw = 0;
    f >> raw;
    if (f.fail())
        return false;

    // The kernel driver returns the value multiplied by 1000
    value = static_cast<float>(raw) / 1000.0f;
    return true;
}

void initDHT11(const string& iio_path)
{
    temp_file = iio_path + "/in_temp_input";
    hum_file = iio_path + "/in_humidityrelative_input";
    initialized = true;
}

bool readDHT11(float& temp, float& hum)
{
    if (!initialized)
        return false;

    const int max_retry = 5;
    // The DHT11 driver needs about 2s between consecutive reads, so we retry a few times
    for (int i = 1; i <= max_retry; i++)
    {
        float t = 0.0f, h = 0.0f;
        bool ok_t = readSensorFile(temp_file, t);
        bool ok_h = readSensorFile(hum_file, h);

        if (ok_t && ok_h)
        {
            temp = t;
            hum = h;
            return true;
        }

        if (i < max_retry)
            this_thread::sleep_for(chrono::seconds(2));
    }
    return false;
}

void cleanupDHT11()
{
    initialized = false;
}