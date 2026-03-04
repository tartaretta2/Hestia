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
        // 1. Apri il chip
    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        std::cerr << "Failed to open GPIO chip" << std::endl;
        return;
    }

    // 2. Crea la configurazione della linea
    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    // Abilita interrupt su entrambi i fronti (salita e discesa)
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

    // 3. Crea il line config e aggiungi il pin
    gpiod_line_config* line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &MS_PIN, 1, settings);

    // 4. Crea il request config (consumer name)
    gpiod_request_config* req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "motion_sensor");

    // 5. Fai la richiesta
    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request) {
        std::cerr << "Failed to request GPIO line" << std::endl;
    }

    // 6. Libera le config (non servono più dopo la request)
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
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
            std::cout << "Movimento rilevato!" << std::endl;
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