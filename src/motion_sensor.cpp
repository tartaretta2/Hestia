#include "motion_sensor.h"
#include "houseControl.h"
#include <iostream>

#ifndef SIM
#include <gpiod.h>
#endif

using namespace std;

#ifndef SIM
static gpiod_chip *alarm_chip = nullptr;
static gpiod_line_request *alarm_req = nullptr;
static gpiod_edge_event_buffer *alarm_buf = nullptr;

static gpiod_chip *lights_chip = nullptr;
static gpiod_line_request *lights_req = nullptr;
static gpiod_edge_event_buffer *lights_buf = nullptr;

bool initAlarmMS(const char *gpioChip, const unsigned int msPin)
{
    cout << "Initializing Alarm Motion Sensor on GPIO chip: " << gpioChip << ", pin: " << msPin << endl;
    alarm_chip = gpiod_chip_open(gpioChip);
    if (!alarm_chip)
    {
        cerr << "Failed to open GPIO chip" << endl;
        return false;
    }

    // configure the pin as an input with edge detection on both edges
    gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

    gpiod_line_config *lineConf = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lineConf, &msPin, 1, settings);

    gpiod_request_config *requestConf = gpiod_request_config_new();
    gpiod_request_config_set_consumer(requestConf, "motion_sensor");

    // request the line from the kernel
    alarm_req = gpiod_chip_request_lines(alarm_chip, requestConf, lineConf);
    if (!alarm_req)
    {
        cerr << "Failed to request MS GPIO line" << endl;
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(lineConf);
        gpiod_request_config_free(requestConf);
        gpiod_chip_close(alarm_chip);
        alarm_chip = nullptr;
        return false;
    }

    alarm_buf = gpiod_edge_event_buffer_new(1);
    if (!alarm_buf)
    {
        cerr << "Failed to create MS edge-event buffer" << endl;
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(lineConf);
        gpiod_request_config_free(requestConf);
        gpiod_line_request_release(alarm_req);
        alarm_req = nullptr;
        gpiod_chip_close(alarm_chip);
        alarm_chip = nullptr;
        return false;
    }

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);

    return true;
}

int readAlarmMS()
{
    if (!alarm_req || !alarm_buf)
        return 0;

    // Wait for the next edge event with a timeout(500ms) so the alarm
    // listener can exit cleanly when the system is disarmed
    int ret = gpiod_line_request_wait_edge_events(alarm_req, 500000000LL); 
    if (ret <= 0)
        return 0;

    ret = gpiod_line_request_read_edge_events(alarm_req, alarm_buf, 1);
    if (ret <= 0)
        return 0;

    gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(alarm_buf, 0);
    if (!event)
        return 0;

    if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE)
    {
        return 1;
    }

    return 0;
}

bool initLightsMS(const char *gpioChip, const unsigned int msPin)
{
    lights_chip = gpiod_chip_open(gpioChip);
    if (!lights_chip) {
        cerr << "LightsMS: Failed to open GPIO chip" << endl;
        return false;
    }

    gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

    gpiod_line_config *lineConf = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lineConf, &msPin, 1, settings);

    gpiod_request_config *reqConf = gpiod_request_config_new();
    gpiod_request_config_set_consumer(reqConf, "lights_motion_sensor");

    lights_req = gpiod_chip_request_lines(lights_chip, reqConf, lineConf);
    if (!lights_req) {
        cerr << "LightsMS: failed to request line" << endl;
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(lineConf);
        gpiod_request_config_free(reqConf);
        gpiod_chip_close(lights_chip);
        lights_chip = nullptr;
        return false;
    }

    lights_buf = gpiod_edge_event_buffer_new(1);
    if (!lights_buf) {
        cerr << "LightsMS: failed to create event buffer" << endl;
        gpiod_line_request_release(lights_req);
        lights_req = nullptr;
        gpiod_chip_close(lights_chip);
        lights_chip = nullptr;
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(lineConf);
        gpiod_request_config_free(reqConf);
        return false;
    }

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(reqConf);

    return true;
}

// Returns 1 on rising edge, -1 on falling edge, 0 on timeout/error.
int readLightsMS()
{
    if (!lights_req || !lights_buf)
        return 0;

    int ret = gpiod_line_request_wait_edge_events(lights_req, 500000000LL);

    if (ret <= 0)
        return 0; // timeout or error: signal no motion

    ret = gpiod_line_request_read_edge_events(lights_req, lights_buf, 1);
    if (ret <= 0)
        return 0;

    gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(lights_buf, 0);
    if (!event)
        return 0;

    return gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE ? 1 : -1;
}

void cleanupAlarmMS()
{
    if (alarm_buf) {
        gpiod_edge_event_buffer_free(alarm_buf);
        alarm_buf = nullptr;
    }
    if (alarm_req) {
        gpiod_line_request_release(alarm_req);
        alarm_req = nullptr;
    }
    if (alarm_chip) {
        gpiod_chip_close(alarm_chip);
        alarm_chip = nullptr;
    }
}

void cleanupLightsMS()
{
    if (lights_buf) {
        gpiod_edge_event_buffer_free(lights_buf);
        lights_buf = nullptr;
    }
    if (lights_req) {
        gpiod_line_request_release(lights_req);
        lights_req = nullptr;
    }
    if (lights_chip) {
        gpiod_chip_close(lights_chip);
        lights_chip = nullptr;
    }
}

#endif

int simulateMS()
{
    return rand() % 2;
}