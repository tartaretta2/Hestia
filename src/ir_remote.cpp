#include "ir_remote.h"
#include "houseControl.h"
#include <iostream>

using namespace std;

extern atomic<bool> running; 

// Key mapping for the remote control buttons
// Key - Name - Action
static const Key KEYS[] = {
    { KEY_POWER,      "POWER",      Action::ShutdownSystem   },
    { KEY_VOL_UP,     "VOL+",       Action::None             },
    { KEY_VOL_DOWN,   "VOL-",       Action::None             },
    { KEY_1,          "1",          Action::LightsToggle     },
    { KEY_2,          "2",          Action::GateToggle       },
    { KEY_3,          "3",          Action::ToggleACMode     },
    { KEY_FUNC_STOP,  "FUNC/STOP",  Action::ToggleLightsMode },
    { KEY_REWIND,     "<<",         Action::None             },
    { KEY_PLAY_PAUSE, "PLAY/PAUSE", Action::AlarmToggle      },
    { KEY_FAST_FWD,   ">>",         Action::None             },
    { KEY_DOWN,       "DOWN",       Action::None             },
    { KEY_UP,         "UP",         Action::None             },
    { KEY_EQ,         "EQ",         Action::None             },
    { KEY_ST_REPT,    "ST/REPT",    Action::None             },
    { KEY_0,          "0",          Action::ToggleHeatingMode},
    { KEY_4,          "4",          Action::ToggleAC         },
    { KEY_5,          "5",          Action::ToggleHeating    },
    { KEY_6,          "6",          Action::None             },
    { KEY_7,          "7",          Action::None             },
    { KEY_8,          "8",          Action::None             },
    { KEY_9,          "9",          Action::None             },
};

//lookup a key by its code and 
//return a pointer to the Key struct or nullptr if not found
const Key *lookupKey(uint8_t code)
{
    for (const auto &k : KEYS)
        if (k.code == code)
            return &k;
    return nullptr;
}

//parse the action enum to a string (for logging)
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
    case Action::ShutdownSystem:
        return "ShutdownSystem";
    case Action::ToggleLightsMode:
        return "ToggleLightsMode";
    case Action::ToggleACMode:
        return "ToggleACMode";
    case Action::ToggleAC:
        return "ToggleAC";
    case Action::ToggleHeatingMode:
        return "ToggleHeatingMode";
    case Action::ToggleHeating:
        return "ToggleHeating";
    case Action::None:
        return "None";
    default:
        return "Unknown";
    }
}

//call function associated to pressed key
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
        cout << "  ->  [Alarm] alarm_toggle()" << endl;
        toggleAlarmActivation();
        break;
    case Action::LightsToggle:
        cout << "  ->  [Lights] lights_toggle()" << endl;
        toggleLightsActivation();
        break;
    case Action::GateToggle:
        cout << "  ->  [Gate] gate_toggle()" << endl;
        toggleGateActivation();
        break;
    case Action::ToggleLightsMode:
        cout << "  ->  [Lights] toggle_lights_mode()" << endl;
        toggleLightsMode();
        break;
    case Action::ToggleACMode:
        cout << "  ->  [AC] toggle_ac_mode()" << endl;
        toggleACMode();
        break;
    case Action::ToggleAC:
        cout << "  ->  [AC] toggle_ac()" << endl;
        toggleAC();
        break;
    case Action::ToggleHeatingMode:
        cout << "  ->  [HEATING] toggle_heating_mode()" << endl;
        toggleHeatingMode();
        break;
    case Action::ToggleHeating:
        cout << "  ->  [HEATING] toggle_heating()" << endl;
        toggleHeating();
        break;
    case Action::ShutdownSystem:
        cout << "  ->  [System] shutdown_system()" << endl;
        running = false; // Signal the main loop to exit
        break;
    default:
        break;
    }
}