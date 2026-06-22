#ifndef DHT11_H
#define DHT11_H

#include <string>


class DHT11 {
public:
  
    DHT11(const std::string& iio_path = "/sys/bus/iio/devices/iio:device0");


    bool read(float& temperature, float& humidity, int max_retry = 5);

private:
    std::string temp_file_;
    std::string hum_file_;

    bool readSensorFile(const std::string& path, float& value);
};



namespace dht11_api {

  
    void initDHT11();


    bool readDHT11(float& temp, float& hum);


    void cleanupDHT11();

}

#endif // DHT11_H