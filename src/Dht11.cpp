#include "Dht11.h"
#include <fstream>
#include <unistd.h>

// =====================================================================
// Implementazione della classe DHT11
// =====================================================================

DHT11::DHT11(const std::string& iio_path)
    : temp_file_(iio_path + "/in_temp_input"),
      hum_file_(iio_path + "/in_humidityrelative_input") {}

bool DHT11::readSensorFile(const std::string& path, float& value) {
    std::ifstream f(path);
    if (!f.is_open()) {
        // File non aprito (es. "Connection timed out" o overlay non caricato)
        return false;
    }
    long raw = 0;
    f >> raw;
    if (f.fail()) {
        return false;
    }
    // Il driver kernel restituisce valori in millesimi (es. 25000 = 25.0 �C)
    value = static_cast<float>(raw) / 1000.0f;
    return true;
}

bool DHT11::read(float& temperature, float& humidity, int max_retry) {
    for (int tentativo = 1; tentativo <= max_retry; tentativo++) {
        float t = 0.0f, h = 0.0f;
        bool ok_t = readSensorFile(temp_file_, t);
        bool ok_h = readSensorFile(hum_file_,  h);

        if (ok_t && ok_h) {
            temperature = t;
            humidity    = h;
            return true;
        }

        // Il driver DHT11 ha bisogno di ~2s tra letture consecutive
        if (tentativo < max_retry) {
            sleep(2);
        }
    }
    return false;
}


namespace dht11_api {

    // Istanza globale del sensore (creata da initDHT11, distrutta da cleanupDHT11)
    static DHT11* g_sensor = nullptr;

    void initDHT11() {
        if (!g_sensor) {
            g_sensor = new DHT11();
        }
    }

    bool readDHT11(float& temp, float& hum) {
        if (!g_sensor) {
            return false;
        }
        return g_sensor->read(temp, hum);
    }

    void cleanupDHT11() {
        delete g_sensor;
        g_sensor = nullptr;
    }

}