#ifndef DHT11_H
#define DHT11_H

#include <string>


class DHT11 {
public:
  
    DHT11(const string& iio_path = "/sys/bus/iio/devices/iio:device0");


    bool read(float& temperature, float& humidity, int max_retry = 5);

private:
    string temp_file_;
    string hum_file_;

    bool readSensorFile(const string& path, float& value);
};

namespace dht11_api {

  
    void initDHT11();


    bool readDHT11(float& temp, float& hum);


    void cleanupDHT11();

}

#endif