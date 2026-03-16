#ifndef __IR_REMOTE_H__
#define __IR_REMOTE_H__

#include <cstdint>

// Codici NEC del telecomando Elegoo
#define KEY_POWER       0x45
#define KEY_VOL_UP      0x46
#define KEY_FUNC_STOP   0x47
#define KEY_REWIND      0x44
#define KEY_PLAY_PAUSE  0x40
#define KEY_FAST_FWD    0x43
#define KEY_DOWN        0x07
#define KEY_VOL_DOWN    0x15
#define KEY_UP          0x09
#define KEY_EQ          0x19
#define KEY_ST_REPT     0x0D
#define KEY_0           0x16
#define KEY_1           0x0C
#define KEY_2           0x18
#define KEY_3           0x5E
#define KEY_4           0x08
#define KEY_5           0x1C
#define KEY_6           0x5A
#define KEY_7           0x42
#define KEY_8           0x52
#define KEY_9           0x4A

// Azioni smart house gestite dal telecomando
enum class Action {
    None,
    AlarmToggle,     // POWER -> accendi/spegni allarme
    LightsToggle,    // 1     -> accendi/spegni luci
    GateToggle,      // 2     -> apri/chiudi cancello
    HeatingToggle,   // 3     -> accendi/spegni riscaldamento
    LightsUp,        // VOL+  -> aumenta luminosit�
    LightsDown,      // VOL-  -> diminuisci luminosit�
    Unknown
};

// Descrittore tasto
struct Key {
    uint8_t code;
    const char* name;
    Action action;
};

// Cerca un tasto nella tabella, ritorna nullptr se non trovato
const Key* lookupKey(uint8_t code);

// Ritorna il nome stringa di un'azione
const char* actionName(Action a);

// Gestisce un tasto: stampa info e chiama l'azione giusta
void handleKey(uint8_t code);

#endif
