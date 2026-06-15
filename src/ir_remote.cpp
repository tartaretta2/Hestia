#include "ir_remote.h"
#include <iostream>

using namespace std;

// Mapping codice -> nome -> azione 
static const Key KEYS[] = {
    { KEY_POWER,      "POWER",      Action::AlarmToggle   },
    { KEY_VOL_UP,     "VOL+",       Action::LightsUp      },
    { KEY_VOL_DOWN,   "VOL-",       Action::LightsDown    },
    { KEY_1,          "1",          Action::LightsToggle  },
    { KEY_2,          "2",          Action::GateToggle    },
    { KEY_3,          "3",          Action::HeatingToggle },
    { KEY_FUNC_STOP,  "FUNC/STOP",  Action::None          },
    { KEY_REWIND,     "<<",         Action::None          },
    { KEY_PLAY_PAUSE, "PLAY/PAUSE", Action::None          },
    { KEY_FAST_FWD,   ">>",         Action::None          },
    { KEY_DOWN,       "DOWN",       Action::None          },
    { KEY_UP,         "UP",         Action::None          },
    { KEY_EQ,         "EQ",         Action::None          },
    { KEY_ST_REPT,    "ST/REPT",    Action::None          },
    { KEY_0,          "0",          Action::None          },
    { KEY_4,          "4",          Action::None          },
    { KEY_5,          "5",          Action::None          },
    { KEY_6,          "6",          Action::None          },
    { KEY_7,          "7",          Action::None          },
    { KEY_8,          "8",          Action::None          },
    { KEY_9,          "9",          Action::None          },
};

const Key* lookupKey(uint8_t code)
{
    for (const auto& k : KEYS)
        if (k.code == code) return &k;
    return nullptr;
}

const char* actionName(Action action)
{
    switch (action) {
        case Action::AlarmToggle:   return "AlarmToggle";
        case Action::LightsToggle:  return "LightsToggle";
        case Action::GateToggle:    return "GateToggle";
        case Action::HeatingToggle: return "HeatingToggle";
        case Action::LightsUp:      return "LightsUp";
        case Action::LightsDown:    return "LightsDown";
        case Action::None:          return "None";
        default:                    return "Unknown";
    }
}

void handleKey(uint8_t code)
{
    const Key* key = lookupKey(code);
    Action action = key ? key->action : Action::Unknown;
    const char* name = key ? key->name : "?";

    cout << "[IR] Key=" << name
         << "  Code=0x" << hex << (int)code << dec
         << "  Action=" << actionName(action) << endl;

    // Da sostituire con le chiamate alle funzioni reali
    switch (action) {
        case Action::AlarmToggle:
            cout << "  -> [TODO] alarm_toggle()" << endl;
            break;
        case Action::LightsToggle:
            cout << "  -> [TODO] lights_toggle()" << endl;
            break;
        case Action::GateToggle:
            cout << "  -> [TODO] gate_toggle()" << endl;
            break;
        case Action::HeatingToggle:
            cout << "  -> [TODO] heating_toggle()" << endl;
            break;
        case Action::LightsUp:
            cout << "  -> [TODO] lights_up()" << endl;
            break;
        case Action::LightsDown:
            cout << "  -> [TODO] lights_down()" << endl;
            break;
        default:
            break;
    }
}
