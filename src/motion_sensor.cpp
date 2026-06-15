#include "motion_sensor.h"
#include <iostream>

#ifndef SIM
    #include <gpiod.h>
#endif

using namespace std;

#ifndef SIM
static gpiod_chip* chip = nullptr;
static gpiod_line_request* request = nullptr;

void initMS(const char* GPIO_CHIP, const unsigned int MS_PIN){
        
    // Open GPIO chip (to be moved to main)
    cout << "Initializing Motion Sensor on GPIO chip: " << GPIO_CHIP << ", pin: " << MS_PIN << endl;
    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        cerr << gpiod_chip_get_info(chip) << endl << "Failed to open GPIO chip" << endl;
        return;
    }

    // 2. Crea la configurazione della linea
    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    // Abilita interrupt su entrambi i fronti (salita e discesa)
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

    // 3. Crea il line config e aggiungi il pin
    gpiod_line_config* lineConf = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lineConf, &MS_PIN, 1, settings);

    // 4. Crea il request config (consumer name)
    gpiod_request_config* requestConf = gpiod_request_config_new();
    gpiod_request_config_set_consumer(requestConf, "motion_sensor");

    // 5. Fai la richiesta
    request = gpiod_chip_request_lines(chip, requestConf, lineConf);
    if (!request) 
        cerr << "Failed to request MS GPIO line" << endl;

    // 6. Libera le config (non servono più dopo la request)
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConf);
    gpiod_request_config_free(requestConf);
}

int readMS(){
    if (!request) return 0;

    // Aspetta un evento con timeout 100ms
    gpiod_edge_event_buffer* buffer = gpiod_edge_event_buffer_new(1);
    int ret = gpiod_line_request_wait_edge_events(request, 100000000); // 100ms in ns

    if (ret == 1) {
        gpiod_line_request_read_edge_events(request, buffer, 1);
        gpiod_edge_event* event = gpiod_edge_event_buffer_get_event(buffer, 0);

        if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE) {
            gpiod_edge_event_buffer_free(buffer);
            return 4;
        }
    }

    gpiod_edge_event_buffer_free(buffer);
    return 0;
}
#endif

int simulateMS(){
    //Motion sensor simulation: > 3 means motion detected
    return rand() % 5;
}