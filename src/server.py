import socket
from flask import Flask, jsonify, render_template_string

app = Flask(__name__)

# Local UDP socket configuration for communication with the C++ backend
CPP_IP = "127.0.0.1"  
CPP_PORT = 12345      

device_states = {
    "alarm_on": False,
    "lights_on": False
}

def sendCommand(action):
    try:
        sckt = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sckt.sendto(action.encode('utf-8'), (CPP_IP, CPP_PORT))
        return {"status": "success", "message": f"Command '{action}' sent successfully."}
    except Exception as e:
        return {"status": "error", "message": str(e)}

@app.route('/api/toggleAlarm', methods=['POST'])
def toggle_alarm():
    res = sendCommand("toggleAlarm")
    if res["status"] == "success":
        device_states["alarm_on"] = not device_states["alarm_on"]
        res["newState"] = device_states["alarm_on"]
    return jsonify(res)

@app.route('/api/toggleLights', methods=['POST'])
def toggle_lights():
    res = sendCommand("toggleLights")
    if res["status"] == "success":
        device_states["lights_on"] = not device_states["lights_on"]
        res["newState"] = device_states["lights_on"]
    return jsonify(res)

@app.route('/api/openGate', methods=['POST'])
def open_gate():
    return jsonify(sendCommand("openGate"))

# Frontend UI
@app.get('/')
def home():
    html_content = """
    <!DOCTYPE html>
    <html lang="it">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Hestia - Remote control</title>
        <script src="https://cdn.tailwindcss.com"></script>
    </head>

    <body class="bg-gray-100 flex flex-col items-center justify-center min-h-screen p-4 font-sans">
        <div class="bg-white p-6 rounded-2xl shadow-xl w-full max-w-md">
            <h1 class="text-2xl font-bold text-center text-gray-800 mb-6">Control panel</h1>
            
            <div class="space-y-4">
                <button id="btnAlarm" onclick="sendCommand('toggleAlarm')" class="w-full bg-red-600 hover:bg-red-700 text-white font-semibold py-3 px-4 rounded-xl transition">
                    {{ '🚨 Turn off alarm' if states.alarm_on else '🚨 Turn on alarm' }}
                </button>
                
                <button id="btnLights" onclick="sendCommand('toggleLights')" class="w-full bg-yellow-500 hover:bg-yellow-600 text-white font-semibold py-3 px-4 rounded-xl transition">
                    {{ '💡 Turn off lights' if states.lights_on else '💡 Turn on lights' }}
                </button>
                
                <button id="btnGate" onclick="sendCommand('openGate')" class="w-full bg-green-600 hover:bg-green-700 text-white font-semibold py-3 px-4 rounded-xl transition">
                    🚪 Open gate
                </button>
            </div>
            
            <label for="state" class="block mt-6 text-sm font-medium text-gray-700 text-center">Current state:</label>
            <div id="state" class="mt-6 p-3 bg-gray-50 border border-gray-200 rounded-xl text-sm text-gray-600 text-center font-mono">
                Ready
            </div>
        </div>

        <script>
            function sendCommand(endpoint) {
                const stateDiv = document.getElementById('state');
                stateDiv.innerText = "Sending command...";
                
                fetch('/api/' + endpoint, { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        if(data.status === 'success') {
                            stateDiv.innerText = `Success.\n ${data.message || endpoint}`;
                            stateDiv.className = "mt-6 p-3 bg-green-50 border border-green-200 rounded-xl text-sm text-green-700 text-center font-mono";
                            
                            if (endpoint === 'toggleAlarm') {
                                document.getElementById('btnAlarm').innerText = data.newState ? '🚨 Turn off alarm' : '🚨 Turn on alarm';
                            } else if (endpoint === 'toggleLights') {
                                document.getElementById('btnLights').innerText = data.newState ? '💡 Turn off lights' : '💡 Turn on lights';
                            }
                        } else {
                            stateDiv.innerText = "Error: " + data.message;
                            stateDiv.className = "mt-6 p-3 bg-red-50 border border-red-200 rounded-xl text-sm text-red-700 text-center font-mono";
                        }
                    })
                    .catch(error => {
                        stateDiv.innerText = "Error: " + error.message;
                        stateDiv.className = "mt-6 p-3 bg-red-50 border border-red-200 rounded-xl text-sm text-red-700 text-center font-mono";
                    });
            }
        </script>
    </body>
    </html>
    """
    return render_template_string(html_content, states=device_states)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5678, debug=True)