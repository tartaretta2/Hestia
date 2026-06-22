import socket
from flask import Flask, jsonify, render_template_string

app = Flask(__name__)

CPP_IP = "127.0.0.1"  
CPP_PORT = 12345      

# Send a command to the C++ server and optionally wait for a reply
def sendCommand(action, expect_reply=False):
    try:
        sckt = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sckt.settimeout(1.0) # If the C++ server doesn't respond within 1 second, it will timeout
        sckt.sendto(action.encode('utf-8'), (CPP_IP, CPP_PORT))
        
        if expect_reply:
            data, _ = sckt.recvfrom(1024)
            return data.decode('utf-8')
        return "OK"
    except socket.timeout:
        return "TIMEOUT"
    except Exception as e:
        return f"ERROR: {str(e)}"

@app.route('/api/getState', methods=['GET'])
def get_state():
    reply = sendCommand("getState", expect_reply=True)
    
    if reply.startswith("ALARM"):
        state = {}
        for part in reply.split('|'): # expected format: "ALARM:1|LIGHTS:0"
            key, val = part.split(':')
            state[key] = True if val == "1" else False
        return jsonify({"status": "success", "data": state})
        
    return jsonify({"status": "error", "message": "Error getting state from C++"})

@app.route('/api/<command>', methods=['POST'])
def send_command(command):
    reply = sendCommand(command, expect_reply=True)
    if reply == "OK":
        return jsonify({"status": "success", "message": "Command executed successfully"})
    return jsonify({"status": "error", "message": "Error communicating with C++"})

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
                    🚨 Loading...
                </button>
                <button id="btnLights" onclick="sendCommand('toggleLights')" class="w-full bg-yellow-500 hover:bg-yellow-600 text-white font-semibold py-3 px-4 rounded-xl transition">
                    💡 Loading...
                </button>
                <button id="btnGate" onclick="sendCommand('openGate')" class="w-full bg-green-600 hover:bg-green-700 text-white font-semibold py-3 px-4 rounded-xl transition">
                    🚪 Open gate
                </button>
            </div>
            
            <div id="connectionStatus" class="mt-6 p-2 text-sm text-center font-bold text-gray-500">
                Connecting...
            </div>
        </div>

        <script>
            function sendCommand(endpoint) {
                fetch('/api/' + endpoint, { method: 'POST' })
                    .then(r => r.json())
                    .then(data => {
                        if(data.status !== 'success') alert("Error: " + data.message);
                        fetchState(); // Force update of the state after sending a command
                    });
            }

            function fetchState() {
                fetch('/api/getState')
                    .then(r => r.json())
                    .then(data => {
                        const statusDiv = document.getElementById('connectionStatus');
                        if(data.status === 'success') {
                            statusDiv.innerText = "🟢 System online";
                            statusDiv.className = "mt-6 p-2 text-sm text-center font-bold text-green-600";
                            
                            document.getElementById('btnAlarm').innerText = data.data.ALARM ? '🚨 Turn off alarm' : '🚨 Turn on alarm';
                            document.getElementById('btnLights').innerText = data.data.LIGHTS ? '💡 Turn off lights' : '💡 Turn on lights';
                        } else {
                            statusDiv.innerText = "🔴 Device offline";
                            statusDiv.className = "mt-6 p-2 text-sm text-center font-bold text-red-600";
                        }
                    })
                    .catch(() => {
                        document.getElementById('connectionStatus').innerText = "🔴 Web Server Error";
                    });
            }

            // start active state polling: fetch the state every 1.5 seconds to keep the UI updated
            setInterval(fetchState, 1500);
            fetchState(); 
        </script>
    </body>
    </html>
    """
    return render_template_string(html_content)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5678, debug=True)