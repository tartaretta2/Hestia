import socket
from flask import Flask, jsonify, render_template_string

app = Flask(__name__)

CPP_IP = "127.0.0.1"  
CPP_PORT = 12345      

# Send a command to the C++ server and optionally wait for a reply
def sendCommand(action, expect_reply=False):
    try:
        sckt = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # If the C++ server doesn't respond within 1 second, it will timeout
        sckt.settimeout(1.0) 
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
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Rajdhani:wght@500;600;700&display=swap');
        .font-tech { font-family: 'Orbitron', sans-serif; }
        .font-panel { font-family: 'Rajdhani', sans-serif; }
    </style>
</head>
<body class="bg-slate-950 text-slate-100 min-h-screen md:h-screen flex flex-col p-4 md:p-6 font-panel selection:bg-cyan-500 selection:text-slate-900 relative overflow-x-hidden md:overflow-hidden">
    
    <div class="absolute inset-0 bg-[linear-gradient(to_right,#0f172a_1px,transparent_1px),linear-gradient(to_bottom,#0f172a_1px,transparent_1px)] bg-[size:4rem_4rem] [mask-image:radial-gradient(ellipse_60%_50%_at_50%_50%,#000_70%,transparent_100%)] opacity-30 pointer-events-none"></div>

    <div class="w-full flex-1 max-w-[1600px] mx-auto flex flex-col gap-6 h-full justify-between relative z-10">
        
        <div class="grid grid-cols-1 md:grid-cols-3 gap-6 md:h-[48%] items-stretch">
            
            <div class="bg-slate-900/25 backdrop-blur-md border border-slate-800/80 border-t-2 border-t-rose-500/40 rounded-3xl p-6 md:p-8 flex flex-col justify-between shadow-[0_-4px_15px_rgba(244,63,94,0.04)] transition-all duration-500 hover:border-t-rose-500 hover:shadow-[0_-10px_25px_rgba(244,63,94,0.2)] group">
                <div>
                    <span class="text-xs font-tech text-rose-500 tracking-widest uppercase opacity-80">01 // Security</span>
                    <h2 class="text-2xl font-bold mt-1 text-slate-200 tracking-wide">Alarm system</h2>
                </div>
                <button id="btnAlarm" onclick="sendCommand('toggleAlarm')" class="w-full bg-slate-800/80 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 hover:text-white tracking-wide font-tech text-sm shadow-md">
                    🚨 Loading...
                </button>
            </div>

            <div class="flex flex-col items-center justify-center p-4 text-center min-h-[250px] md:min-h-0 select-none">
                <h1 class="text-5xl md:text-6xl font-black tracking-[0.15em] text-transparent bg-clip-text bg-gradient-to-r from-cyan-400 via-teal-400 to-blue-500 font-tech drop-shadow-[0_0_20px_rgba(34,211,238,0.2)]">HESTIA</h1>
                <p class="text-xs uppercase tracking-[0.3em] text-slate-400 font-tech mt-2">Smart home control</p>
                
                <button id="btnShutdown" onclick="sendCommand('shutdownSystem')" class="group/btn mt-6 bg-slate-900/80 hover:bg-rose-950/40 border border-slate-800 hover:border-rose-500/70 size-14 rounded-full flex items-center justify-center transition-all duration-300 relative shadow-lg shadow-black/50">
                    <svg class="w-5 h-5 text-slate-400 group-hover/btn:text-rose-400 transition-colors duration-300" fill="none" stroke="currentColor" stroke-width="2.5" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" d="M5.636 5.636a9 9 0 1012.728 0M12 3v9" />
                    </svg>
                    <span class="absolute -bottom-7 text-[9px] font-tech text-rose-400 tracking-wider font-bold opacity-0 group-hover/btn:opacity-100 transition-opacity duration-300 whitespace-nowrap">SHUTDOWN</span>
                </button>

                <div id="connectionStatus" class="bg-slate-900/90 border border-slate-800 px-6 py-2 rounded-full text-[10px] font-tech tracking-widest text-slate-500 uppercase shadow-inner mt-8 min-w-[240px] whitespace-nowrap">
                    Initializing Connection...
                </div>
            </div>

            <div class="bg-slate-900/25 backdrop-blur-md border border-slate-800/80 border-t-2 border-t-emerald-500/40 rounded-3xl p-6 md:p-8 flex flex-col justify-between shadow-[0_-4px_15px_rgba(16,185,129,0.04)] transition-all duration-500 hover:border-t-emerald-500 hover:shadow-[0_-10px_25px_rgba(16,185,129,0.2)] group">
                <div>
                    <span class="text-xs font-tech text-emerald-500 tracking-widest uppercase opacity-80">02 // Perimeter</span>
                    <h2 class="text-2xl font-bold mt-1 text-slate-200 tracking-wide">Main Gate</h2>
                </div>
                <button id="btnGate" onclick="sendCommand('toggleGate')" class="w-full bg-slate-800/80 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 hover:text-white tracking-wide font-tech text-sm shadow-md">
                    🚪 Loading...
                </button>
            </div>

        </div>

        <div class="grid grid-cols-1 md:grid-cols-3 gap-6 md:h-[48%] items-stretch">
            
            <div class="bg-slate-900/25 backdrop-blur-md border border-slate-800/80 border-t-2 border-t-amber-500/40 rounded-3xl p-6 md:p-8 flex flex-col justify-between shadow-[0_-4px_15px_rgba(245,158,11,0.04)] transition-all duration-500 hover:border-t-amber-500 hover:shadow-[0_-10px_25px_rgba(245,158,11,0.2)] group">
                <div>
                    <span class="text-xs font-tech text-amber-500 tracking-widest uppercase opacity-80">03 // Illumination</span>
                    <h2 class="text-2xl font-bold mt-1 text-slate-200 tracking-wide">Lighting</h2>
                </div>
                
                <div class="space-y-4 mt-4">
                    <div class="flex items-center justify-between bg-slate-950/60 p-3 rounded-xl border border-slate-800/60">
                        <span class="text-sm font-medium text-slate-400">Automatic mode</span>
                        <label class="relative inline-flex items-center cursor-pointer select-none">
                            <input type="checkbox" id="switchLightMode" onclick="sendCommand('toggleLightsMode')" class="sr-only peer">
                            <div class="w-11 h-6 bg-slate-800 rounded-full peer peer-focus:ring-0 peer-checked:after:translate-x-full peer-checked:after:border-slate-950 after:content-[''] after:absolute after:top-[2px] after:start-[2px] after:bg-slate-400 peer-checked:after:bg-slate-950 after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-amber-500 border border-slate-700"></div>
                        </label>
                        <span id="btnToggleLightMode" class="hidden"></span>
                    </div>
                    <button id="btnLights" onclick="sendCommand('toggleLights')" class="w-full bg-slate-800/80 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs shadow-md">
                        💡 Loading...
                    </button>
                </div>
            </div>

            <div class="bg-slate-900/25 backdrop-blur-md border border-slate-800/80 border-t-2 border-t-cyan-500/40 rounded-3xl p-6 md:p-8 flex flex-col justify-between shadow-[0_-4px_15px_rgba(6,182,212,0.04)] transition-all duration-500 hover:border-t-cyan-500 hover:shadow-[0_-10px_25px_rgba(6,182,212,0.2)] group">
                <div>
                    <span class="text-xs font-tech text-cyan-500 tracking-widest uppercase opacity-80">04 // Climatization</span>
                    <h2 class="text-2xl font-bold mt-1 text-slate-200 tracking-wide">Air Conditioning</h2>
                </div>
                
                <div class="space-y-4 mt-4">
                    <div class="flex items-center justify-between bg-slate-950/60 p-3 rounded-xl border border-slate-800/60">
                        <span class="text-sm font-medium text-slate-400">Automatic mode</span>
                        <label class="relative inline-flex items-center cursor-pointer select-none">
                            <input type="checkbox" id="switchACMode" onclick="sendCommand('toggleACMode')" class="sr-only peer">
                            <div class="w-11 h-6 bg-slate-800 rounded-full peer peer-focus:ring-0 peer-checked:after:translate-x-full peer-checked:after:border-slate-950 after:content-[''] after:absolute after:top-[2px] after:start-[2px] after:bg-slate-400 peer-checked:after:bg-slate-950 after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-cyan-500 border border-slate-700"></div>
                        </label>
                        <span id="btnToggleACMode" class="hidden"></span>
                    </div>
                    <button id="btnAC" onclick="sendCommand('toggleAC')" class="w-full bg-slate-800/80 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs shadow-md">
                        ❄️ Loading...
                    </button>
                </div>
            </div>

            <div class="bg-slate-900/25 backdrop-blur-md border border-slate-800/80 border-t-2 border-t-orange-500/40 rounded-3xl p-6 md:p-8 flex flex-col justify-between shadow-[0_-4px_15px_rgba(249,115,22,0.04)] transition-all duration-500 hover:border-t-orange-500 hover:shadow-[0_-10px_25px_rgba(249,115,22,0.2)] group">
                <div>
                    <span class="text-xs font-tech text-orange-400 tracking-widest uppercase opacity-80">05 //  Heating system</span>
                    <h2 class="text-2xl font-bold mt-1 text-slate-200 tracking-wide">Heating</h2>
                </div>
                
                <div class="space-y-4 mt-4">
                    <div class="flex items-center justify-between bg-slate-950/60 p-3 rounded-xl border border-slate-800/60">
                        <span class="text-sm font-medium text-slate-400">Automatic mode</span>
                        <label class="relative inline-flex items-center cursor-pointer select-none">
                            <input type="checkbox" id="switchHeatingMode" onclick="sendCommand('toggleHeatingMode')" class="sr-only peer">
                            <div class="w-11 h-6 bg-slate-800 rounded-full peer peer-focus:ring-0 peer-checked:after:translate-x-full peer-checked:after:border-slate-950 after:content-[''] after:absolute after:top-[2px] after:start-[2px] after:bg-slate-400 peer-checked:after:bg-slate-950 after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-orange-500 border border-slate-700"></div>
                        </label>
                        <span id="btnToggleHeatingMode" class="hidden"></span>
                    </div>
                    <button id="btnHeating" onclick="sendCommand('toggleHeating')" class="w-full bg-slate-800/80 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs shadow-md">
                        🔥 Loading...
                    </button>
                </div>
            </div>

        </div>

    </div>

    <script>
        function sendCommand(endpoint) {
            fetch('/api/' + endpoint, { method: 'POST' })
                .then(r => r.json())
                .then(data => {
                    if(data.status !== 'success') alert("Error: " + data.message);
                    fetchState(); 
                });
        }

        function fetchState() {
            fetch('/api/getState')
                .then(r => r.json())
                .then(data => {
                    const statusDiv = document.getElementById('connectionStatus');
                    if(data.status === 'success') {
                        statusDiv.innerText = "🟢 SYSTEM LINK ACTIVE";
                        statusDiv.className = "bg-slate-900/90 border border-emerald-500/30 px-6 py-2 rounded-full text-[10px] font-tech tracking-widest text-emerald-400 shadow-[0_0_15px_rgba(16,185,129,0.1)] text-center mt-8 min-w-[240px] whitespace-nowrap";
                        
                        // --- ALLARME ---
                        const alarmBtn = document.getElementById('btnAlarm');
                        if (data.data.ALARM) {
                            alarmBtn.innerText = '🚨 DISARM ALARM';
                            alarmBtn.className = "w-full bg-rose-600/20 hover:bg-rose-600/30 border border-rose-500 text-rose-200 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-sm shadow-[0_0_20px_rgba(244,63,94,0.15)]";
                        } else {
                            alarmBtn.innerText = '🛡️ ARM ALARM';
                            alarmBtn.className = "w-full bg-slate-800/60 hover:bg-slate-800 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-sm";
                        }
                        
                        // --- CANCELLO ---
                        const gateBtn = document.getElementById('btnGate');
                        if (data.data.GATE) {
                            gateBtn.innerText = '🚪 CLOSE GATE';
                            gateBtn.className = "w-full bg-emerald-600/20 hover:bg-emerald-600/30 border border-emerald-500 text-emerald-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-sm shadow-[0_0_20px_rgba(16,185,129,0.15)]";
                        } else {
                            gateBtn.innerText = '🚪 OPEN GATE';
                            gateBtn.className = "w-full bg-slate-800/60 hover:bg-slate-800 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-sm";
                        }

                        // --- LUCI ---
                        document.getElementById('switchLightMode').checked = !data.data.LMODE; 
                        document.getElementById('btnToggleLightMode').innerText = data.data.LMODE ? 'Motion' : 'Manual'; 

                        const lightsBtn = document.getElementById('btnLights');
                        lightsBtn.innerText = data.data.LIGHTS ? '💡 TURN OFF LIGHTS' : '💡 TURN ON LIGHTS';
                        if(data.data.LMODE) {
                            if (data.data.LIGHTS) {
                                lightsBtn.className = "w-full bg-amber-500/20 border border-amber-500 text-amber-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs shadow-[0_0_15px_rgba(245,158,11,0.15)]";
                            } else {
                                lightsBtn.className = "w-full bg-slate-800/60 hover:bg-slate-800 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs";
                            }
                            lightsBtn.disabled = false;
                        } else {
                            lightsBtn.className = "w-full bg-slate-950 border border-slate-900 text-slate-600 font-semibold py-4 rounded-xl tracking-wide font-tech text-xs opacity-40 cursor-not-allowed";
                            lightsBtn.disabled = true;
                        }

                        // --- ARIA CONDIZIONATA ---
                        document.getElementById('switchACMode').checked = !data.data.ACMODE;
                        document.getElementById('btnToggleACMode').innerText = data.data.ACMODE ? 'Auto' : 'Manual';

                        const acBtn = document.getElementById('btnAC');
                        acBtn.innerText = data.data.AC ? '❄️ AC OFF' : '❄️ AC ON';
                        if(data.data.ACMODE) {
                            if (data.data.AC) {
                                acBtn.className = "w-full bg-cyan-500/20 border border-cyan-500 text-cyan-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs shadow-[0_0_15px_rgba(6,182,212,0.15)]";
                            } else {
                                acBtn.className = "w-full bg-slate-800/60 hover:bg-slate-800 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs";
                            }
                            acBtn.disabled = false;
                        } else {
                            acBtn.className = "w-full bg-slate-950 border border-slate-900 text-slate-600 font-semibold py-4 rounded-xl tracking-wide font-tech text-xs opacity-40 cursor-not-allowed";
                            acBtn.disabled = true;
                        }

                        // --- RISCALDAMENTO ---
                        if (data.data.HEATINGMODE !== undefined) {
                            document.getElementById('switchHeatingMode').checked = !data.data.HEATINGMODE;
                            document.getElementById('btnToggleHeatingMode').innerText = data.data.HEATINGMODE ? 'Auto' : 'Manual';
                            
                            const heatBtn = document.getElementById('btnHeating');
                            heatBtn.innerText = data.data.HEATING ? '🔥 HEATING OFF' : '🔥 HEATING ON';
                            if(data.data.HEATINGMODE) {
                                if (data.data.HEATING) {
                                    heatBtn.className = "w-full bg-orange-500/20 border border-orange-500 text-orange-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs shadow-[0_0_15px_rgba(249,115,22,0.15)]";
                                } else {
                                    heatBtn.className = "w-full bg-slate-800/60 hover:bg-slate-800 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl transition-all duration-300 tracking-wide font-tech text-xs";
                                }
                                heatBtn.disabled = false;
                            } else {
                                heatBtn.className = "w-full bg-slate-950 border border-slate-900 text-slate-600 font-semibold py-4 rounded-xl tracking-wide font-tech text-xs opacity-40 cursor-not-allowed";
                                heatBtn.disabled = true;
                            }
                        } else {
                            document.getElementById('btnHeating').innerText = "🔥 HEATING READY";
                            document.getElementById('btnHeating').className = "w-full bg-slate-800/60 border border-slate-700 text-slate-300 font-semibold py-4 rounded-xl tracking-wide font-tech text-xs";
                        }

                    } else {
                        statusDiv.innerText = "🚨 NODE OFFLINE";
                        statusDiv.className = "bg-slate-900/90 border border-rose-500/50 px-6 py-2 rounded-full text-[10px] font-tech tracking-widest text-rose-500 shadow-[0_0_15px_rgba(239,68,68,0.2)] text-center mt-8 min-w-[240px] whitespace-nowrap";
                    }
                })
                .catch(() => {
                    document.getElementById('connectionStatus').innerText = "📡 SERVER DISCONNECTED";
                    document.getElementById('connectionStatus').className = "bg-slate-900/90 border border-amber-500/50 px-6 py-2 rounded-full text-[10px] font-tech tracking-widest text-amber-500 text-center mt-8 min-w-[240px] whitespace-nowrap";
                });
        }

        setInterval(fetchState, 1500);
        fetchState(); 
    </script>
</body>
</html>
    """
    return render_template_string(html_content)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5678, debug=True)