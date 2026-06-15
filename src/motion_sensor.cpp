#include "motion_sensor.h"
#include "houseControl.h"
#include <iostream>

#ifndef SIM
#include <gpiod.h>
#endif

using namespace std;

#ifndef SIM
static gpiod_chip *chip = nullptr;
static gpiod_line_request *request = nullptr;

// Initialize the motion sensor GPIO line for edge detection.
void initMS(const char *gpioChip, const unsigned int msPin)
{

    // Open GPIO chip (this is the underlying device for pins)
    cout << "Initializing Motion Sensor on GPIO chip: " << gpioChip << ", pin: " << msPin << endl;
    chip = gpiod_chip_open(gpioChip);
    if (!chip)
    {
        cerr << "Failed to open GPIO chip" << endl;
        return;
    }

    // Configure the pin as an input with edge detection
    gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    // Enable interrupts on both rising and falling edges
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

    // Add the desired pin to the configuration
    gpiod_line_config *lineConf = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lineConf, &msPin, 1, settings);

    // Create request configuration (consumer name used for debugging)
    gpiod_request_config *requestConf = gpiod_request_config_new();
    gpiod_request_config_set_consumer(requestConf, "motion_sensor");

    // Request the line(s) from the kernel
    request = gpiod_chip_request_lines(chip, requestConf, lineConf);
    if (!request)
        cerr << "Failed to request MS GPIO line" << endl;

    // Free temporary config structures (no longer needed)
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);
}

int readMS()
{
    if (!request)
        return 0;

    gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);

    // Aspetta un evento con timeout 100ms
    int ret = gpiod_line_request_wait_edge_events(request, 100000000); // 100ms in ns

    if (ret == 1)
    {
        gpiod_line_request_read_edge_events(request, buffer, 1);
        gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(buffer, 0);

        if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE)
        {
            gpiod_edge_event_buffer_free(buffer);
            return 4;
        }
    }

    gpiod_edge_event_buffer_free(buffer);
    return 0;
}

void cleanupMS(){
    if(request) gpiod_line_request_release(request);
    if(chip) gpiod_chip_close(chip);
}

#endif

int simulateMS()
{
    // Motion sensor simulation: > 3 means motion detected
    return rand() % 5;
}