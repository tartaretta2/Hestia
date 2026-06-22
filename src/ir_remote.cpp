#include "ir_remote.h"
#include "houseControl.h"
#include <iostream>

using namespace std;

// Key mapping for the remote control buttons
// Key - Name - Action
static const Key KEYS[] = {
    { KEY_POWER,      "POWER",      Action::ShutdownSystem},
    { KEY_VOL_UP,     "VOL+",       Action::LightsUp      },
    { KEY_VOL_DOWN,   "VOL-",       Action::LightsDown    },
    { KEY_1,          "1",          Action::LightsToggle  },
    { KEY_2,          "2",          Action::GateToggle    },
    { KEY_3,          "3",          Action::HeatingToggle },
    { KEY_FUNC_STOP,  "FUNC/STOP",  Action::None          },
    { KEY_REWIND,     "<<",         Action::None          },
    { KEY_PLAY_PAUSE, "PLAY/PAUSE", Action::AlarmToggle   },
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

const Key *lookupKey(uint8_t code)
{
    for (const auto &k : KEYS)
        if (k.code == code)
            return &k;
    return nullptr;
}

const char *actionName(Action action)
{
    switch (action)
    {
    case Action::AlarmToggle:
        return "AlarmToggle";
    case Action::LightsToggle:
        return "LightsToggle";
    case Action::GateToggle:
        return "GateToggle";
    case Action::HeatingToggle:
        return "HeatingToggle";
    case Action::LightsUp:
        return "LightsUp";
    case Action::LightsDown:
        return "LightsDown";
    case Action::SirenToggle:
        return "SirenToggle";
    case Action::ShutdownSystem:
        return "ShutdownSystem";
    case Action::None:
        return "None";
    default:
        return "Unknown";
    }
}

void handleKey(uint8_t code)
{
    const Key *key = lookupKey(code);
    Action action = key ? key->action : Action::Unknown;
    const char *name = key ? key->name : "?";

    cout << "[IR] Key=" << name
         << "  Code=0x" << hex << (int)code << dec
         << "  Action=" << actionName(action) << endl;

    switch (action)
    {
    case Action::AlarmToggle:
        cout << "  ->  alarm_toggle()" << endl;
        toggleAlarmActivation();
        break;
    case Action::LightsToggle:
        cout << "  -> [TODO] lights_toggle()" << endl;
        break;
    case Action::GateToggle:
        cout << "  ->  gate_toggle()" << endl;
        toggleGateActivation();
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
    case Action::SirenToggle:
        cout << "  ->  siren_toggle()" << endl;
        toggleSiren();
        break;
    case Action::ShutdownSystem:
        cout << "  ->  shutdown_system()" << endl;
        shutdownSystem();
        break;
    default:
        break;
    }
}