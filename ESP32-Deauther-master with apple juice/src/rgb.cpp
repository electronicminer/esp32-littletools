#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Adafruit_NeoPixel.h>

extern WebServer server;
extern WebSocketsServer webSocket;
extern Adafruit_NeoPixel strip;

// 定义颜色结构体
struct Color {
  int r, g, b;
} currentColor = {255, 0, 0};

// 函数声明
String getValue(String data, char separator, int index);
void fadeToColor(int targetR, int targetG, int targetB, int steps = 50, int delay_ms = 20);

void handle_rgb() {
    String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>RGB Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdn.jsdelivr.net/npm/@jaames/iro@5.5.2/dist/iro.min.js"></script>
    <style>
      body {
        background-color: #f0f0f0;
        margin: 0;
        padding: 20px;
        font-family: Arial, sans-serif;
      }
      .container { 
        width: 300px; 
        margin: 20px auto;
        text-align: center;
        background-color: white;
        padding: 20px;
        border-radius: 10px;
        box-shadow: 0 2px 5px rgba(0,0,0,0.1);
      }
      .brightness-slider {
        width: 100%;
        margin: 20px 0;
      }
      .presets {
        margin: 20px 0;
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
        gap: 5px;
      }
      .preset-btn {
        width: 40px;
        height: 40px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
      }
      #save-preset {
        width: auto;
        padding: 10px;
        background-color: #4CAF50;
        color: white;
      }
      .brightness-label {
        display: block;
        margin: 10px 0;
      }
      #power-btn {
        width: auto;
        padding: 10px 20px;
        margin: 10px 0;
        background-color: #f44336;
        color: white;
        border: none;
        border-radius: 5px;
        cursor: pointer;
      }
      .back-btn {
        display: block;
        width: 100%;
        padding: 10px;
        background-color: #3498db;
        color: white;
        text-decoration: none;
        border-radius: 5px;
        text-align: center;
        margin-top: 20px;
        border: none;
        cursor: pointer;
        font-size: 16px;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <div id="color-picker"></div>
      <button id="power-btn" onclick="togglePower()">关灯</button>
      <label class="brightness-label">亮度: <span id="brightness-value">100%</span></label>
      <input type="range" class="brightness-slider" id="brightness" min="10" max="100" value="100" step="1">
      <div class="presets" id="presets">
        <button id="save-preset" onclick="savePreset()">保存当前颜色</button>
      </div>
      <form action="/" method="get">
        <button type="submit" class="back-btn">返回主页</button>
      </form>
    </div>
    
  <script>
    function throttle(func, limit) {
      let inThrottle;
      return function(...args) {
        if (!inThrottle) {
          func.apply(this, args);
          inThrottle = true;
          setTimeout(() => inThrottle = false, limit);
        }
      }
    }

    let ws = null;
    let wsReconnectTimer = null;
    let isUpdatingFromWebSocket = false;
    let isUpdatingFromBrightness = false;
    let originalColor = {r: 255, g: 0, b: 0};
    let currentColor = {r: 255, g: 0, b: 0};
    let brightness = 100;
    let debounceTimer;
    let isPowerOn = true;
    let savedColor = {r: 255, g: 0, b: 0};
    
    function initWebSocket() {
      ws = new WebSocket(`ws://${window.location.hostname}:81`);
      ws.onopen = function() { console.log('WebSocket Connected'); };
      ws.onclose = function() {
        console.log('WebSocket Disconnected');
        if (!wsReconnectTimer) wsReconnectTimer = setInterval(initWebSocket, 5000);
      };
      ws.onmessage = function(event) {
        if (!isUpdatingFromBrightness && !isUpdatingFromWebSocket) {
          isUpdatingFromWebSocket = true;
          const [r, g, b] = event.data.split(',').map(Number);
          if (brightness >= 10) {
            const factor = 100 / brightness;
            originalColor = {
              r: Math.min(255, Math.round(r * factor)),
              g: Math.min(255, Math.round(g * factor)),
              b: Math.min(255, Math.round(b * factor))
            };
            colorPicker.color.rgb = originalColor;
          }
          currentColor = {r, g, b};
          setTimeout(() => { isUpdatingFromWebSocket = false; }, 100);
        }
      };
    }
    
    function togglePower() {
      isPowerOn = !isPowerOn;
      const powerBtn = document.getElementById('power-btn');
      if (!isPowerOn) {
        savedColor = {...currentColor};
        sendColor({r: 0, g: 0, b: 0}, true);
        powerBtn.textContent = '开灯';
        powerBtn.style.backgroundColor = '#4CAF50';
        document.getElementById('brightness').disabled = true;
        colorPicker.disable();
      } else {
        sendColor(savedColor, true);
        powerBtn.textContent = '关灯';
        powerBtn.style.backgroundColor = '#f44336';
        document.getElementById('brightness').disabled = false;
        colorPicker.enable();
      }
    }
    
    initWebSocket();

    const colorPicker = new iro.ColorPicker("#color-picker", {
      width: 280,
      color: "#ff0000",
      layout: [{ component: iro.ui.Wheel, options: {} }],
      borderWidth: 2,
      borderColor: "#333"
    });

    function updateColor(color) {
      if (!isUpdatingFromWebSocket && isPowerOn) {
        originalColor = {...color.rgb};
        applyBrightness();
      }
    }

    function applyBrightness() {
      if (!isPowerOn) return;
      isUpdatingFromBrightness = true;
      const factor = brightness / 100;
      const adjustedColor = {
        r: Math.max(0, Math.round(originalColor.r * factor)),
        g: Math.max(0, Math.round(originalColor.g * factor)),
        b: Math.max(0, Math.round(originalColor.b * factor))
      };
      clearTimeout(debounceTimer);
      debounceTimer = setTimeout(() => {
        sendColor(adjustedColor);
        currentColor = adjustedColor;
        isUpdatingFromBrightness = false;
      }, 50);
    }

    function sendColor(color, withTransition = false) {
      if (!isPowerOn && !withTransition) return;
      const params = `rgb=${color.r},${color.g},${color.b}${withTransition ? '&transition=1' : ''}`;
      fetch(`/setColor?${params}`).catch(console.error);
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(`${color.r},${color.g},${color.b}`);
      }
    }

    function savePreset() {
      const presetData = {
        hex: colorPicker.color.hexString,
        rgb: {...originalColor}
      };
      const presets = JSON.parse(localStorage.getItem('colorPresets') || '[]');
      if (presets.length >= 8) presets.shift();
      presets.push(presetData);
      localStorage.setItem('colorPresets', JSON.stringify(presets));
      updatePresetButtons();
    }

    function updatePresetButtons() {
      const presetsContainer = document.getElementById('presets');
      const saveButton = document.getElementById('save-preset');
      presetsContainer.innerHTML = '';
      presetsContainer.appendChild(saveButton);
      const presets = JSON.parse(localStorage.getItem('colorPresets') || '[]');
      presets.forEach(preset => {
        const btn = document.createElement('button');
        btn.className = 'preset-btn';
        btn.style.backgroundColor = preset.hex;
        btn.onclick = () => {
          if (isPowerOn) {
            originalColor = {...preset.rgb};
            colorPicker.color.rgb = preset.rgb;
            applyBrightness();
          }
        };
        presetsContainer.appendChild(btn);
      });
    }

    const throttledColorUpdate = throttle(color => updateColor(color), 100);
    colorPicker.on("color:change", throttledColorUpdate);
    
    const throttledBrightnessUpdate = throttle(value => {
      brightness = Math.max(10, value);
      document.getElementById('brightness-value').textContent = brightness + '%';
      applyBrightness();
    }, 100);

    document.getElementById('brightness').addEventListener('input', e => {
      throttledBrightnessUpdate(parseInt(e.target.value));
    });

    

    updatePresetButtons();
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handle_setColor() {
  String rgb = server.arg("rgb");
  bool transition = server.hasArg("transition");

  int r = getValue(rgb, ',', 0).toInt();
  int g = getValue(rgb, ',', 1).toInt();
  int b = getValue(rgb, ',', 2).toInt();

  if (transition) {
    fadeToColor(r, g, b);
  } else {
    currentColor = {r, g, b};
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
  }

  server.send(200, "text/plain", "OK");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      {
        String colorMsg = String(currentColor.r) + "," + 
                         String(currentColor.g) + "," + 
                         String(currentColor.b);
        webSocket.sendTXT(num, colorMsg);
      }
      break;
    case WStype_TEXT:
      webSocket.broadcastTXT(payload, length);
      break;
  }
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;
  for(int i = 0; i <= maxIndex && found <= index; i++) {
    if(data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void fadeToColor(int targetR, int targetG, int targetB, int steps, int delay_ms) {
  float stepR = (targetR - currentColor.r) / (float)steps;
  float stepG = (targetG - currentColor.g) / (float)steps;
  float stepB = (targetB - currentColor.b) / (float)steps;
  
  float r = currentColor.r;
  float g = currentColor.g;
  float b = currentColor.b;
  
  for(int i = 0; i < steps; i++) {
    r += stepR;
    g += stepG;
    b += stepB;
    strip.setPixelColor(0, strip.Color((int)r, (int)g, (int)b));
    strip.show();
    delay(delay_ms);
  }
  
  currentColor = {targetR, targetG, targetB};
}