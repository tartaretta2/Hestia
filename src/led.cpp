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
static gpiod_chip *alarm_chip = nullptr;
static gpiod_line_request *alarm_req = nullptr;
static gpiod_chip *lights_chip = nullptr;
static gpiod_line_request *lights_req = nullptr;
static gpiod_chip *AC_chip = nullptr;
static gpiod_line_request *AC_req = nullptr;
static gpiod_chip *heating_chip = nullptr;
static gpiod_line_request *heating_req = nullptr;

static void initOneLED(const char* gpioChip, unsigned int ledPin, const char* consumer, gpiod_chip** chip_out, gpiod_line_request** req_out)
{
    cout << "Initializing LED (" << consumer << ") on " << gpioChip << ", pin " << ledPin << endl;

    *chip_out = gpiod_chip_open(gpioChip);
    if (!*chip_out) {
        cerr << "Failed to open GPIO chip for " << consumer << endl;
        return;
    }

    gpiod_line_settings* s = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(s, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(s, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_config* lc = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lc, &ledPin, 1, s);

    gpiod_request_config* rc = gpiod_request_config_new();
    gpiod_request_config_set_consumer(rc, consumer);

    *req_out = gpiod_chip_request_lines(*chip_out, rc, lc);
    if (!*req_out)
        cerr << "Failed to request GPIO line for " << consumer << endl;

    gpiod_line_settings_free(s);
    gpiod_line_config_free(lc);
    gpiod_request_config_free(rc);
}

void initLEDs(const char* gpioChip, unsigned int alarmPin, unsigned int lightsPin, unsigned int acPin, unsigned int heatingPin)
{
    initOneLED(gpioChip, alarmPin,   "led",         &alarm_chip,   &alarm_req);
    initOneLED(gpioChip, lightsPin,  "lights_led",  &lights_chip,  &lights_req);
    initOneLED(gpioChip, acPin,      "AC_led",      &AC_chip,      &AC_req);
    initOneLED(gpioChip, heatingPin, "heating_led", &heating_chip, &heating_req);
}

void toggleAlarmLED(const unsigned int ledPin)
{
    if (!alarm_req)
        return;

    int i;
    for (i = 0; i < 5; ++i)
    {
        gpiod_line_request_set_value(alarm_req, ledPin, GPIOD_LINE_VALUE_ACTIVE);
        this_thread::sleep_for(chrono::milliseconds(70));
        gpiod_line_request_set_value(alarm_req, ledPin, GPIOD_LINE_VALUE_INACTIVE);
        this_thread::sleep_for(chrono::milliseconds(30));
    }
}

void setLED(const unsigned int ledPin, bool on)
{
    if (ledPin == ALARM_LED)
    {
        if (!alarm_req)
            return;
        if (on)
            gpiod_line_request_set_value(alarm_req, ledPin, GPIOD_LINE_VALUE_ACTIVE);
        else
            gpiod_line_request_set_value(alarm_req, ledPin, GPIOD_LINE_VALUE_INACTIVE);
    }
    else if (ledPin == LIGHTS_LED)
    {
        if (!lights_req)
            return;
        if (on)
            gpiod_line_request_set_value(lights_req, ledPin, GPIOD_LINE_VALUE_ACTIVE);
        else
            gpiod_line_request_set_value(lights_req, ledPin, GPIOD_LINE_VALUE_INACTIVE);
    }
    else if (ledPin == AC_LED)
    {
        if (!AC_req)
            return;
        if (on)
            gpiod_line_request_set_value(AC_req, ledPin, GPIOD_LINE_VALUE_ACTIVE);
        else
            gpiod_line_request_set_value(AC_req, ledPin, GPIOD_LINE_VALUE_INACTIVE);
    }
    else if (ledPin == HEATING_LED)
    {
        if (!heating_req)
            return;
        if (on)
            gpiod_line_request_set_value(heating_req, ledPin, GPIOD_LINE_VALUE_ACTIVE);
        else
            gpiod_line_request_set_value(heating_req, ledPin, GPIOD_LINE_VALUE_INACTIVE);
    }
}

void cleanupLEDs()
{
    setLED(ALARM_LED, false);
    setLED(LIGHTS_LED, false);
    setLED(AC_LED, false);
    setLED(HEATING_LED, false);
    if (alarm_req)
        gpiod_line_request_release(alarm_req);
    if (alarm_chip)
        gpiod_chip_close(alarm_chip);
    if (lights_req)
        gpiod_line_request_release(lights_req);
    if (lights_chip)
        gpiod_chip_close(lights_chip);
    if (AC_req)
        gpiod_line_request_release(AC_req);
    if (AC_chip)
        gpiod_chip_close(AC_chip);
    if (heating_req)
        gpiod_line_request_release(heating_req);
    if (heating_chip)
        gpiod_chip_close(heating_chip);
}

#endif

void simulateLED(bool on)
{
    cout << "LED " << (on ? "ON" : "OFF") << endl;
}