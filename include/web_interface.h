#pragma once

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>LED Planter Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background: #f0f0f0;
        }
        .card {
            background: white;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .slider {
            width: 100%;
            margin: 10px 0;
        }
        .button {
            background: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            margin: 5px;
        }
        .button:hover {
            background: #45a049;
        }
        .color-picker {
            margin: 10px 0;
        }
        #effects-list {
            margin: 10px 0;
        }
        .status {
            margin-top: 10px;
            padding: 10px;
            border-radius: 4px;
            display: none;
        }
        .status.error {
            background: #ffebee;
            color: #c62828;
            display: block;
        }
        .status.success {
            background: #e8f5e9;
            color: #2e7d32;
            display: block;
        }
        input[type="text"], input[type="password"], input[type="number"] {
            width: 100%;
            padding: 8px;
            margin: 5px 0;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        .settings-group {
            margin-bottom: 15px;
        }
        .settings-group h3 {
            margin-top: 0;
            color: #333;
        }
    </style>
</head>
<body>
    <div class="card">
        <h2>LED Control</h2>
        <div>
            <label>Brightness:</label>
            <input type="range" min="0" max="255" value="50" class="slider" id="brightness">
        </div>
        <div>
            <label>Color:</label>
            <input type="color" id="color" class="color-picker">
        </div>
    </div>

    <div class="card">
        <h2>Effects</h2>
        <select id="effects-list">
            <option value="solid">Solid Color</option>
            <option value="rainbow">Rainbow</option>
            <option value="ripple">Water Ripple</option>
            <option value="twinkle">Twinkle</option>
            <option value="wave">Color Wave</option>
        </select>
        <button class="button" onclick="applyEffect()">Apply Effect</button>
    </div>

    <div class="card">
        <h2>Settings</h2>
        <div class="settings-group">
            <h3>Device Setup</h3>
            <div>
                <label>Hostname:</label>
                <input type="text" id="hostname" placeholder="led-planter">
                <button class="button" onclick="saveHostname()">Save Hostname</button>
                <div id="hostname-status" class="status"></div>
            </div>
        </div>

        <div class="settings-group">
            <h3>WiFi Setup</h3>
            <div>
                <label>SSID:</label>
                <input type="text" id="wifi-ssid">
                <label>Password:</label>
                <input type="password" id="wifi-password">
                <button class="button" onclick="saveWifi()">Save WiFi Settings</button>
                <div id="wifi-status" class="status"></div>
            </div>
        </div>

        <div class="settings-group">
            <h3>MQTT Setup</h3>
            <div>
                <label>Broker Host:</label>
                <input type="text" id="mqtt-host">
                <label>Port:</label>
                <input type="number" id="mqtt-port" value="1883">
                <label>Username:</label>
                <input type="text" id="mqtt-user">
                <label>Password:</label>
                <input type="password" id="mqtt-password">
                <button class="button" onclick="saveMQTT()">Save MQTT Settings</button>
                <div id="mqtt-status" class="status"></div>
            </div>
        </div>
    </div>

    <script>
        function updateBrightness(value, updateDevice = true) {
            if (updateDevice) {
                fetch('/brightness?value=' + value);
            }
            document.getElementById('brightness').value = value;
        }

        function updateColor(color, updateDevice = true) {
            if (updateDevice) {
                fetch('/color?value=' + color.substring(1));
            }
            document.getElementById('color').value = color;
        }

        function updateEffect(effect, updateDevice = true) {
            if (updateDevice && effect) {
                fetch('/effect?name=' + effect);
            }
            if (effect) {
                document.getElementById('effects-list').value = effect;
            }
        }

        function applyEffect() {
            var effect = document.getElementById('effects-list').value;
            updateEffect(effect);
        }

        function loadCurrentState() {
            fetch('/get-state')
                .then(response => response.json())
                .then(data => {
                    updateBrightness(data.brightness, false);
                    updateColor(data.color, false);
                    updateEffect(data.effect, false);
                })
                .catch(error => console.error('Error loading state:', error));
        }

        function saveWifi() {
            var ssid = document.getElementById('wifi-ssid').value;
            var password = document.getElementById('wifi-password').value;
            var statusDiv = document.getElementById('wifi-status');
            
            if (!ssid) {
                statusDiv.textContent = 'Please enter SSID';
                statusDiv.className = 'status error';
                return;
            }

            statusDiv.textContent = 'Saving WiFi settings...';
            statusDiv.className = 'status';
            
            fetch('/setup-wifi', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    ssid: ssid,
                    password: password
                })
            })
            .then(response => response.text())
            .then(data => {
                statusDiv.textContent = data;
                statusDiv.className = 'status success';
                if (data.includes('Rebooting')) {
                    setTimeout(() => {
                        statusDiv.textContent = 'Device is rebooting. Please wait 30 seconds and then connect to your WiFi network.';
                    }, 2000);
                }
            })
            .catch(error => {
                statusDiv.textContent = 'Error saving WiFi settings';
                statusDiv.className = 'status error';
            });
        }

        function saveMQTT() {
            var host = document.getElementById('mqtt-host').value;
            var port = document.getElementById('mqtt-port').value;
            var user = document.getElementById('mqtt-user').value;
            var password = document.getElementById('mqtt-password').value;
            var statusDiv = document.getElementById('mqtt-status');
            
            if (!host) {
                statusDiv.textContent = 'Please enter MQTT broker host';
                statusDiv.className = 'status error';
                return;
            }

            statusDiv.textContent = 'Saving MQTT settings...';
            statusDiv.className = 'status';
            
            fetch('/setup-mqtt', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    host: host,
                    port: parseInt(port),
                    user: user,
                    password: password
                })
            })
            .then(response => response.text())
            .then(data => {
                statusDiv.textContent = data;
                statusDiv.className = 'status success';
                if (data.includes('Rebooting')) {
                    setTimeout(() => {
                        statusDiv.textContent = 'Device is rebooting with new MQTT settings...';
                    }, 2000);
                }
            })
            .catch(error => {
                statusDiv.textContent = 'Error saving MQTT settings';
                statusDiv.className = 'status error';
            });
        }

        function saveHostname() {
            var hostname = document.getElementById('hostname').value;
            var statusDiv = document.getElementById('hostname-status');
            
            if (!hostname) {
                statusDiv.textContent = 'Please enter a hostname';
                statusDiv.className = 'status error';
                return;
            }

            // Validate hostname format
            if (!/^[a-zA-Z0-9-]+$/.test(hostname)) {
                statusDiv.textContent = 'Hostname can only contain letters, numbers, and hyphens';
                statusDiv.className = 'status error';
                return;
            }

            statusDiv.textContent = 'Saving hostname...';
            statusDiv.className = 'status';
            
            fetch('/setup-hostname', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    hostname: hostname
                })
            })
            .then(response => response.text())
            .then(data => {
                statusDiv.textContent = data;
                statusDiv.className = 'status success';
                if (data.includes('Rebooting')) {
                    setTimeout(() => {
                        statusDiv.textContent = 'Device is rebooting. Please wait 30 seconds and then connect using the new hostname.';
                    }, 2000);
                }
            })
            .catch(error => {
                statusDiv.textContent = 'Error saving hostname';
                statusDiv.className = 'status error';
            });
        }

        // Load current settings and state
        fetch('/get-settings')
            .then(response => response.json())
            .then(data => {
                if (data.mqtt) {
                    document.getElementById('mqtt-host').value = data.mqtt.host || '';
                    document.getElementById('mqtt-port').value = data.mqtt.port || 1883;
                    document.getElementById('mqtt-user').value = data.mqtt.user || '';
                }
                if (data.hostname) {
                    document.getElementById('hostname').value = data.hostname;
                }
            });

        // Load initial state
        loadCurrentState();

        // Add event listeners
        document.getElementById('brightness').addEventListener('change', function() {
            updateBrightness(this.value);
        });
        document.getElementById('color').addEventListener('change', function() {
            updateColor(this.value);
        });

        // Refresh state every 5 seconds
        setInterval(loadCurrentState, 5000);
    </script>
</body>
</html>
)rawliteral";
