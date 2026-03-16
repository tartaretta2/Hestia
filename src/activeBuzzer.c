#include "buzzer.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifndef SIM
    #include <gpiod.h>
#endif

using namespace std;

#ifndef SIM
static gpiod_chip* chip = nullptr;
static gpiod_line_request* request = nullptr;

void initBuzzer(const char* GPIO_CHIP, const unsigned int BUZZER_PIN){
    
    cout << "Initializing Buzzer on GPIO chip: " << GPIO_CHIP << ", pin: " << BUZZER_PIN << endl;
    chip = gpiod_chip_open(GPIO_CHIP);
    if(!chip){
        cerr << gpiod_chip_get_info(chip) << endl << "Failed to open GPIO chip" << endl;
        return;
    }

    //Configure line as output and set to low
    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_config* lineConf = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lineConf, &BUZZER_PIN, 1, settings);

    gpiod_request_config* requestConf = gpiod_request_config_new();
    gpiod_request_config_set_consumer(requestConf, "buzzer");

    request = gpiod_chip_request_lines(chip, requestConf, lineConf);
    if(!request)
        cerr << "Failed to request buzzer GPIO line" << endl;

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);
}

void toggleBuzzer(const unsigned int BUZZER_PIN){
    if(!request) return;

    int i;
    for(i = 0; i < 5; ++i){
        gpiod_line_request_set_value(request, BUZZER_PIN, GPIOD_LINE_VALUE_ACTIVE);
        this_thread::sleep_for(chrono::milliseconds(70));
        gpiod_line_request_set_value(request, BUZZER_PIN, GPIOD_LINE_VALUE_INACTIVE);
        this_thread::sleep_for(chrono::milliseconds(30));
    }
}

void cleanupBuzzer(const unsigned int BUZZER_PIN){
    if(request) gpiod_line_request_release(request);
    if(chip) gpiod_chip_close(chip);
}

#endif

void simulateBuzzer(){
    cout << "BUZZER ON" << endl;
}