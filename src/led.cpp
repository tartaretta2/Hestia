#include "led.h"
#include "houseControl.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifndef SIM
#include <gpiod.h>
#endif

using namespace std;

#ifndef SIM
static gpiod_chip *chip = nullptr;
static gpiod_line_request *request = nullptr;

void initLED(const char *gpioChip, const unsigned int ledPin)
{
    cout << "Initializing LED on GPIO chip: " << gpioChip << ", pin: " << ledPin << endl;
    chip = gpiod_chip_open(gpioChip);
    if (!chip)
    {
        cerr << gpiod_chip_get_info(chip) << endl
             << "Failed to open GPIO chip" << endl;
        return;
    }

    // Configure line as output and set to low
    gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_config *lineConf = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lineConf, &ledPin, 1, settings);

    gpiod_request_config *requestConf = gpiod_request_config_new();
    gpiod_request_config_set_consumer(requestConf, "led");

    request = gpiod_chip_request_lines(chip, requestConf, lineConf);
    if (!request)
        cerr << "Failed to request LED GPIO line" << endl;

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);
}

void toggleLED(const unsigned int ledPin){
    if (!request) return;

    int i;
    for(i = 0; i < 5; ++i){
        gpiod_line_request_set_value(request, ledPin, GPIOD_LINE_VALUE_ACTIVE);
        this_thread::sleep_for(chrono::milliseconds(70));
        gpiod_line_request_set_value(request, ledPin, GPIOD_LINE_VALUE_INACTIVE);
        this_thread::sleep_for(chrono::milliseconds(30));
    }
}

void cleanupLED(){
    if(request) gpiod_line_request_release(request);
    if(chip) gpiod_chip_close(chip);
}

#endif

void simulateLED()
{
    cout << "LED ON" << endl;
}