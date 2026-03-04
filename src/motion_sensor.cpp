#include "motion_sensor.h"
#include <iostream>

#ifndef SIM
    #include <gpiod.h>
#endif

using namespace std;

#ifndef SIM
static gpiod_chip* chip = nullptr;
static gpiod_line* line = nullptr;

void initMS(int GPIO_CHIP, int MS_PIN){
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) {
        cerr << "Failed to open GPIO chip" << endl;
        return;
    }
    line = gpiod_chip_get_line(chip, MS_PIN);
    if (!line) {
        cerr << "Failed to get GPIO line" << endl;
        gpiod_chip_close(chip);
        return;
    }

    gpiod_line_request_config config = {
        .consumer     = "motion_sensor",
        .request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES,
        .flags        = 0
    };

    if (gpiod_line_request(line, &config, 0) < 0) {
        std::cerr << "Errore configurazione interrupt" << std::endl;
    }
}

int readMS(){
    if(!line) return 0;

    timespec timeout = {1, 0}; 
    int ret = gpiod_line_event_wait(line, &timeout);

    if(ret == 1){
        gpiod_line_event event;
        gpiod_line:event_read(line, &event);

        if(event.event_type == GPIOD_LIN_EVENT_RISING_EDGE){
            return 4; //motion detected
        }
    }

    return 0;
}
#endif

int simulateMS(){
    //Motion sensor simulation: > 3 means motion detected
    return rand() % 5;
}