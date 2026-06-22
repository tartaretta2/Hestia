#ifndef DHT11_H
#define DHT11_H
 
#include <string>
 
class DHT11 {
public:
    // Costruttore: di default usa /sys/bus/iio/devices/iio:device0
    DHT11(const std::string& iio_path = "/sys/bus/iio/devices/iio:device0");
 
    // Legge temperatura (�C) e umidit� (%).
    // Ritorna true se la lettura � valida, false altrimenti.
    // Effettua fino a max_retry tentativi con pausa tra l'uno e l'altro.
    bool read(float& temperature, float& humidity, int max_retry = 5);
 
private:
    std::string temp_file_;
    std::string hum_file_;
 
    bool readSensorFile(const std::string& path, float& value);
};
 
#endif 