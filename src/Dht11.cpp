#include "Dht11.h"
#include <fstream>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace std;

static string temp_file;
static string hum_file;
static bool initialized = false;

// Reads a single value (in millesimi) from a kernel iio sysfs file
static bool readSensorFile(const string& path, float& value)
{
    ifstream f(path);
    if (!f.is_open())
    {
        // File non aperto (es. "Connection timed out" o overlay non caricato)
        return false;
    }
    long raw = 0;
    f >> raw;
    if (f.fail())
        return false;

    // Il driver kernel restituisce valori in millesimi (es. 25000 = 25.0 C)
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
    for (int tentativo = 1; tentativo <= max_retry; tentativo++)
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

        // Il driver DHT11 ha bisogno di ~2s tra letture consecutive
        if (tentativo < max_retry)
            this_thread::sleep_for(chrono::seconds(2));
    }
    return false;
}

void cleanupDHT11()
{
    initialized = false;
}