void initWebServer() {
    server.on("/", []() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 温湿度控制中心</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css">
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary: #4f46e5;
            --primary-dark: #4338ca;
            --accent: #ec4899;
            --success: #22c55e;
            --warning: #eab308;
            --danger: #ef4444;
            --neutral: #64748b;
            --bg-light: #f8fafc;
            --bg-dark: #f1f5f9;
            --text-primary: #0f172a;
            --shadow-sm: 0 4px 6px -1px rgba(0,0,0,0.1);
            --shadow-md: 0 20px 25px -5px rgba(0,0,0,0.1);
        }
        body {
            font-family: 'Inter', sans-serif;
            background: linear-gradient(to bottom right, var(--bg-light), var(--bg-dark));
            color: var(--text-primary);
            margin: 0;
            padding: 1.5rem 1rem;
            min-height: 100vh;
            display: flex;
            align-items: flex-start;
            justify-content: center;
        }
        .container {
            width: 100%;
            max-width: 420px;
            display: flex;
            flex-direction: column;
            gap: 1.25rem;
        }
        .card {
            background: white;
            border-radius: 1.25rem;
            padding: 1.25rem;
            box-shadow: var(--shadow-md);
            transition: transform 0.25s ease;
        }
        .card:hover { transform: translateY(-3px); }
        .card-header {
            display: flex;
            align-items: center;
            gap: 0.625rem;
            margin-bottom: 1rem;
            font-size: 1.125rem;
            font-weight: 700;
            color: var(--primary);
        }
        .value-display {
            text-align: center;
            margin: 0.75rem 0;
        }
        .value {
            font-size: 2.75rem;
            font-weight: 700;
            line-height: 1;
        }
        .unit {
            font-size: 1.125rem;
            color: var(--neutral);
            margin-left: 0.25rem;
        }
        .status {
            font-size: 0.95rem;
            margin-top: 0.375rem;
            color: var(--neutral);
        }
        .good { color: var(--success); }
        .warn { color: var(--warning); }
        .bad { color: var(--danger); }
        .form-group {
            margin-bottom: 1rem;
            text-align: center;
        }
        label {
            display: block;
            font-size: 0.875rem;
            font-weight: 600;
            margin-bottom: 0.375rem;
            color: var(--neutral);
        }
        input[type="number"] {
            width: 140px;
            padding: 0.625rem;
            font-size: 1.125rem;
            text-align: center;
            border: 1px solid #e0e7ff;
            border-radius: 0.75rem;
            margin: 0 auto;
            display: block;
        }
        input[type="number"]:focus {
            border-color: var(--primary);
            box-shadow: 0 0 0 3px rgba(79,70,229,0.1);
            outline: none;
        }
        .button-group {
            display: flex;
            gap: 0.75rem;
            justify-content: center;
            flex-wrap: wrap;
            margin-top: 0.75rem;
        }
        button {
            padding: 0.625rem 1.25rem;
            font-size: 0.95rem;
            font-weight: 600;
            border-radius: 0.75rem;
            cursor: pointer;
            transition: all 0.2s ease;
            min-width: 110px;
            border: none;
        }
        .btn-primary, .btn-active {
            background: var(--primary);
            color: white;
        }
        .btn-primary:hover, .btn-active:hover {
            background: var(--primary-dark);
        }
        .btn-secondary, .btn-inactive {
            background: #e0e7ff;
            color: var(--primary);
        }
        .btn-secondary:hover, .btn-inactive:hover {
            background: #c7d2fe;
        }
        .status-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 1.25rem;
            margin-top: 1rem;
        }
        .status-item {
            background: var(--bg-light);
            border-radius: 1rem;
            padding: 1rem;
            text-align: center;
            font-size: 0.875rem;
            box-shadow: var(--shadow-sm);
        }
        .status-item strong {
            display: block;
            font-weight: 600;
            margin-bottom: 0.25rem;
        }
        @media (max-width: 480px) {
            .container { padding: 1rem; }
            .value { font-size: 2.25rem; }
            .button-group { flex-direction: column; gap: 0.625rem; }
            .status-grid { grid-template-columns: 1fr; gap: 1rem; }
            input[type="number"] { width: 120px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- 当前环境 -->
        <div class="card">
            <div class="card-header"><i class="fas fa-chart-line"></i> 当前环境</div>
            <div style="display:flex; justify-content:center; gap:3rem; flex-wrap:wrap;">
                <div class="value-display">
                    <div class="value" id="temp">--</div>
                    <span class="unit">℃</span>
                    <div class="status" id="tempStatus"></div>
                </div>
                <div class="value-display">
                    <div class="value" id="hum">--</div>
                    <span class="unit">%</span>
                    <div class="status" id="humStatus"></div>
                </div>
            </div>
        </div>

        <!-- 目标设定 -->
        <div class="card">
            <div class="card-header"><i class="fas fa-sliders-h"></i> 目标设定</div>
            <div class="form-group">
                <label>目标温度 (℃)</label>
                <input id="tset" type="number" step="0.5" value="26">
            </div>
            <div class="form-group">
                <label>目标湿度 (%)</label>
                <input id="hset" type="number" step="1" value="55">
            </div>
            <div style="text-align:center;">
                <button class="btn-primary" onclick="setTarget()">应用设置</button>
            </div>
        </div>

        <!-- 运行模式 -->
        <div class="card">
            <div class="card-header"><i class="fas fa-toggle-on"></i> 运行模式</div>
            <div class="value-display">
                <div class="value" id="mode" style="font-size:1.75rem; margin:0.5rem 0;">--</div>
            </div>
            <div class="button-group">
                <button id="mode-auto" class="btn-primary" onclick="setMode(1)">自动</button>
                <button id="mode-manual" class="btn-secondary" onclick="setMode(0)">手动</button>
            </div>
        </div>

        <!-- 总电源 -->
        <div class="card">
            <div class="card-header"><i class="fas fa-power-off"></i> 总电源</div>
            <div class="value-display">
                <div class="value" id="totalpower" style="font-size:1.75rem; margin:0.5rem 0;">--</div>
            </div>
            <div class="button-group">
                <button id="total-on" class="btn-primary" onclick="setRelay('totalpower',1)">开启</button>
                <button id="total-off" class="btn-secondary" onclick="setRelay('totalpower',0)">关闭</button>
            </div>
        </div>

        <!-- 空调控制器 -->
        <div class="card">
            <div class="card-header"><i class="fas fa-snowflake"></i> 空调控制器</div>
            <div class="status-grid">
                <div class="status-item">
                    <strong>空调电源</strong>
                    <span id="acpower">--</span>
                </div>
                <div class="status-item">
                    <strong>空调模式</strong>
                    <span id="acmode">--</span>
                </div>
            </div>
            <div class="button-group">
                <button id="ac-on" class="btn-primary" onclick="setRelay('acpower',1)">空调 ON</button>
                <button id="ac-off" class="btn-secondary" onclick="setRelay('acpower',0)">OFF</button>
            </div>
            <div class="button-group">
                <button id="ac-cool" class="btn-primary" onclick="setRelay('acmode',1)">制冷</button>
                <button id="ac-heat" class="btn-secondary" onclick="setRelay('acmode',0)">制热</button>
            </div>
        </div>

        <!-- 湿度控制器 -->
        <div class="card">
            <div class="card-header"><i class="fas fa-tint"></i> 湿度控制器</div>
            <div class="status-grid">
                <div class="status-item">
                    <strong>湿度电源</strong>
                    <span id="humpower">--</span>
                </div>
                <div class="status-item">
                    <strong>湿度模式</strong>
                    <span id="humimode">--</span>
                </div>
            </div>
            <div class="button-group">
                <button id="hum-on" class="btn-primary" onclick="setRelay('humpower',1)">湿度 ON</button>
                <button id="hum-off" class="btn-secondary" onclick="setRelay('humpower',0)">OFF</button>
            </div>
            <div class="button-group">
                <button id="hum-add" class="btn-primary" onclick="setRelay('hum',1)">加湿</button>
                <button id="hum-dry" class="btn-secondary" onclick="setRelay('hum',0)">除湿</button>
            </div>
        </div>
    </div>

    <script>
        // 更新按钮活跃状态
        function updateButtonState(id, active) {
            const btn = document.getElementById(id);
            if (btn) {
                if (active) {
                    btn.classList.remove('btn-secondary', 'btn-inactive');
                    btn.classList.add('btn-primary', 'btn-active');
                } else {
                    btn.classList.remove('btn-primary', 'btn-active');
                    btn.classList.add('btn-secondary', 'btn-inactive');
                }
            }
        }

        function loadData() {
            fetch('/get')
                .then(res => res.json())
                .then(d => {
                    document.getElementById('temp').innerText = d.temperature.toFixed(1);
                    document.getElementById('hum').innerText = d.humidity.toFixed(1);
                    
                    const tDiff = d.temperature - d.targetTemp;
                    const hDiff = d.humidity - d.targetHum;
                    document.getElementById('tempStatus').innerText = tDiff > 0 ? '偏高 ↑' : tDiff < 0 ? '偏低 ↓' : '正常 ✓';
                    document.getElementById('tempStatus').className = 'status ' + (Math.abs(tDiff) > 3 ? 'bad' : Math.abs(tDiff) > 1.5 ? 'warn' : 'good');
                    
                    document.getElementById('humStatus').innerText = hDiff > 0 ? '偏湿 ↑' : hDiff < 0 ? '偏干 ↓' : '正常 ✓';
                    document.getElementById('humStatus').className = 'status ' + (Math.abs(hDiff) > 15 ? 'bad' : Math.abs(hDiff) > 8 ? 'warn' : 'good');

                    document.getElementById('mode').innerText = d.auto ? '自动模式' : '手动模式';
                    updateButtonState('mode-auto', d.auto);
                    updateButtonState('mode-manual', !d.auto);

                    document.getElementById('totalpower').innerText = d.totalpower ? '开启' : '关闭';
                    updateButtonState('total-on', d.totalpower);
                    updateButtonState('total-off', !d.totalpower);

                    document.getElementById('acpower').innerText = d.acpower ? '开启' : '关闭';
                    updateButtonState('ac-on', d.acpower);
                    updateButtonState('ac-off', !d.acpower);

                    document.getElementById('acmode').innerText = d.acpower ? (d.acmode ? '制冷' : '制热') : '--';
                    updateButtonState('ac-cool', d.acpower && d.acmode);
                    updateButtonState('ac-heat', d.acpower && !d.acmode);

                    document.getElementById('humpower').innerText = d.humpower ? '开启' : '关闭';
                    updateButtonState('hum-on', d.humpower);
                    updateButtonState('hum-off', !d.humpower);

                    document.getElementById('humimode').innerText = d.humpower ? (d.hum ? '加湿' : '除湿') : '--';
                    updateButtonState('hum-add', d.humpower && d.hum);
                    updateButtonState('hum-dry', d.humpower && !d.hum);
                })
                .catch(err => console.error('数据加载失败:', err));
        }

        function setTarget() {
            let t = document.getElementById('tset').value;
            let h = document.getElementById('hset').value;
            fetch('/set?t=' + t + '&h=' + h).then(() => loadData());
        }

        function setMode(m) {
            fetch('/mode?auto=' + m).then(() => loadData());
        }

        function setRelay(name, v) {
            fetch('/relay?name=' + name + '&on=' + v).then(() => loadData());
        }

        setInterval(loadData, 2000);
        loadData();
    </script>
</body>
</html>
)rawliteral";
        server.send(200, "text/html", html);
    });

    // 以下保持不变（/get, /set, /mode, /relay 路由）
    server.on("/get", []() {
        String json = "{";
        json += "\"temperature\":" + String(currentTemp, 1) + ",";
        json += "\"humidity\":" + String(currentHum, 1) + ",";
        json += "\"targetTemp\":" + String(targetTemp, 1) + ",";
        json += "\"targetHum\":" + String(targetHum, 0) + ",";
        json += "\"auto\":" + String(autoMode ? 1 : 0) + ",";
        json += "\"totalpower\":" + String(totalPowerOn ? 1 : 0) + ",";
        json += "\"acpower\":" + String(acPowerOn ? 1 : 0) + ",";
        json += "\"acmode\":" + String(acCoolMode ? 1 : 0) + ",";
        json += "\"humpower\":" + String(humPowerOn ? 1 : 0) + ",";
        json += "\"hum\":" + String(humiAddMode ? 1 : 0);
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/set", []() {
        if (server.hasArg("t") && server.hasArg("h")) {
            targetTemp = server.arg("t").toFloat();
            targetHum = server.arg("h").toFloat();
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "参数错误");
        }
    });

    server.on("/mode", []() {
        if (server.hasArg("auto")) {
            autoMode = server.arg("auto").toInt() == 1;
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "参数错误");
        }
    });

    server.on("/relay", []() {
        if (server.hasArg("name") && server.hasArg("on")) {
            String name = server.arg("name");
            bool on = server.arg("on").toInt() == 1;
            if (name == "totalpower") totalPowerOn = on;
            if (name == "acpower") acPowerOn = on;
            if (name == "acmode") acCoolMode = on;
            if (name == "humpower") humPowerOn = on;
            if (name == "hum") humiAddMode = on;
            if (name == "acpower" || name == "acmode") lastAcToggleTime = millis();
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "参数错误");
        }
    });

    server.begin();
    Serial.println("Web server started");
}