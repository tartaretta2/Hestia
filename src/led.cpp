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

void initAlarmLED(const char *gpioChip, const unsigned int ledPin)
{
    cout << "Initializing Alarm LED on GPIO chip: " << gpioChip << ", pin: " << ledPin << endl;
    alarm_chip = gpiod_chip_open(gpioChip);
    if (!alarm_chip)
    {
        cerr << gpiod_chip_get_info(alarm_chip) << endl
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

    alarm_req = gpiod_chip_request_lines(alarm_chip, requestConf, lineConf);
    if (!alarm_req)
        cerr << "Failed to request LED GPIO line" << endl;

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);
}

void initLightsLED(const char *gpioChip, const unsigned int ledPin)
{
    cout << "Initializing Lights LED on GPIO chip: " << gpioChip << ", pin: " << ledPin << endl;
    lights_chip = gpiod_chip_open(gpioChip);
    if (!lights_chip)
    {
        cerr << gpiod_chip_get_info(lights_chip) << endl
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
    gpiod_request_config_set_consumer(requestConf, "lights_led");

    lights_req = gpiod_chip_request_lines(lights_chip, requestConf, lineConf);
    if (!lights_req)
        cerr << "Failed to request Lights LED GPIO line" << endl;

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);
}

void initACLED(const char *gpioChip, const unsigned int ledPin)
{
    cout << "Initializing Temperature LED on GPIO chip: " << gpioChip << ", pin: " << ledPin << endl;
    AC_chip = gpiod_chip_open(gpioChip);
    if (!AC_chip)
    {
        cerr << gpiod_chip_get_info(AC_chip) << endl
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
    gpiod_request_config_set_consumer(requestConf, "AC_led");

    AC_req = gpiod_chip_request_lines(AC_chip, requestConf, lineConf);
    if (!AC_req)
        cerr << "Failed to request Temperature LED GPIO line" << endl;

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);
}

void initHeatingLED(const char *gpioChip, const unsigned int ledPin)
{
    cout << "Initializing Temperature LED on GPIO chip: " << gpioChip << ", pin: " << ledPin << endl;
    heating_chip = gpiod_chip_open(gpioChip);
    if (!heating_chip)
    {
        cerr << gpiod_chip_get_info(heating_chip) << endl
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
    gpiod_request_config_set_consumer(requestConf, "heating_led");

    heating_req = gpiod_chip_request_lines(heating_chip, requestConf, lineConf);
    if (!heating_req)
        cerr << "Failed to request Temperature LED GPIO line" << endl;

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);
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