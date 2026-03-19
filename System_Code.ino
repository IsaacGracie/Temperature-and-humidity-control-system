#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <WebServer.h>

// ===== 硬件引脚定义 =====
#define DHTPIN 25
#define DHTTYPE DHT22
#define OLED_RESET -1
#define I2C_SDA 21
#define I2C_SCL 22

// ===== 5 路继电器（高电平触发）=====
#define RELAY_AC_POWER    27
#define RELAY_AC_MODE     26
#define RELAY_HUM_POWER   33
#define RELAY_HUM_MODE    32
#define RELAY_TOTAL_POWER 12

#define RELAY_ON  HIGH
#define RELAY_OFF LOW

// ===== 物理按键引脚（按下接VCC → 高电平触发，使用内部下拉）=====
#define BTN_POWER   23  // 按键1: Power / 确认 / 开关
#define BTN_MODE    19  // 按键2: Mode / 菜单切换 / 进入菜单
#define BTN_UP      4   // 按键3: + / 上 / 加
#define BTN_DOWN    17  // 按键4: - / 下 / 减

// ===== LED 报警灯 =====
#define LED_ALARM_PIN 5

// ===== WiFi AP 配置 =====
const char* AP_SSID = "ESP32_TH";
const char* AP_PASS = "12345678";

// ===== 最小开停机时间（空调压缩机保护） =====
const unsigned long MIN_ON_OFF_TIME = 3UL * 60UL * 1000UL; // 3分钟

// ===== 按键相关 =====
const unsigned long DEBOUNCE_TIME   = 50;     // 去抖时间 ms
const unsigned long LONG_PRESS_TIME = 1500;   // 长按判断阈值 ms
const unsigned long REPEAT_TIME     = 200;    // 长按重复触发间隔 ms

// ===== 全局对象 =====
Adafruit_SH1106G display(128, 64, &Wire, OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// ===== 全局变量 =====
float currentTemp = 0.0;
float currentHum  = 0.0;
bool sensorError = true;
bool oledOK = false;

float targetTemp = 26.0;
float targetHum  = 55.0;
bool autoMode = true;

// 继电器逻辑状态
bool acPowerOn    = false;
bool acCoolMode   = true;    // true=制冷
bool humPowerOn   = true;
bool humiAddMode  = true;    // true=加湿
bool totalPowerOn = true;

// 防频繁启停
unsigned long lastAcToggleTime = 0;

// 按键结构体
struct Button {
    int pin;
    bool lastState          = LOW;
    bool pressed            = false;
    unsigned long pressTime = 0;
    unsigned long lastRepeatTime = 0;
    bool longPressedHandled = false;

    Button(int p) : pin(p) {}
};

// 按键对象
Button btnPower(BTN_POWER);
Button btnMode (BTN_MODE);
Button btnUp   (BTN_UP);
Button btnDown (BTN_DOWN);

// 菜单系统
bool inMenu = false;
int currentMenuItem = 0;
const int MENU_ITEMS = 7;
String menuLabels[MENU_ITEMS] = {
    "AUTO/MAN", "TGT TEMP", "TGT HUM", "AC POWER", "AC MODE", "HUM POWER", "HUM MODE"
};

// 函数原型声明
void readSensorData();
void checkAlarmAndLed();
void controlRelays();
void applyRelayOutputs();
bool canToggleAC();
void handleButtons();
void enterMenu();
void exitMenu();
void adjustMenuItem(int delta);
void updateOLEDDisplay();
void initWebServer();

void setup() {
    delay(1500);
    Serial.begin(115200);
    Serial.println("\n==== ESP32 BOOT OK ====");

    // 继电器初始化
    pinMode(RELAY_AC_POWER, OUTPUT);
    pinMode(RELAY_AC_MODE, OUTPUT);
    pinMode(RELAY_HUM_POWER, OUTPUT);
    pinMode(RELAY_HUM_MODE, OUTPUT);
    pinMode(RELAY_TOTAL_POWER, OUTPUT);

    digitalWrite(RELAY_AC_POWER, RELAY_OFF);
    digitalWrite(RELAY_AC_MODE, RELAY_OFF);
    digitalWrite(RELAY_HUM_POWER, RELAY_ON);
    digitalWrite(RELAY_HUM_MODE, RELAY_OFF);
    digitalWrite(RELAY_TOTAL_POWER, RELAY_ON);

    // 按键初始化
    pinMode(BTN_POWER, INPUT_PULLDOWN);
    pinMode(BTN_MODE, INPUT_PULLDOWN);
    pinMode(BTN_UP, INPUT_PULLDOWN);
    pinMode(BTN_DOWN, INPUT_PULLDOWN);

    // LED 报警灯初始化
    pinMode(LED_ALARM_PIN, OUTPUT);
    digitalWrite(LED_ALARM_PIN, LOW);  // 开机默认灭
    Serial.println("LED_ALARM_PIN (GPIO 5) 初始化完成，默认熄灭");

    Wire.begin(I2C_SDA, I2C_SCL);
    if (display.begin(0x3C, true)) {
        oledOK = true;
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(0, 0);
        display.print("ESP32");
        display.setCursor(0, 32);
        display.print("BOOT");
        display.display();
        Serial.println("OLED OK");
    } else {
        Serial.println("OLED INIT FAIL");
    }

    dht.begin();
    Serial.println("DHT INIT DONE");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

    initWebServer();

    lastAcToggleTime = millis();
    Serial.println("SETUP DONE");
}

void loop() {
    static unsigned long lastRead = 0;
    if (millis() - lastRead >= 2000) {
        readSensorData();
        checkAlarmAndLed();
        controlRelays();
        applyRelayOutputs();
        updateOLEDDisplay();
        lastRead = millis();
    }

    handleButtons();
    server.handleClient();
}

// ===== 菜单函数 =====
void enterMenu() { inMenu = true; currentMenuItem = 0; }
void exitMenu() { inMenu = false; }

void adjustMenuItem(int delta) {
    switch (currentMenuItem) {
        case 0: autoMode = !autoMode; break;
        case 1: targetTemp = constrain(targetTemp + delta * 1.0, 16, 35); break;
        case 2: targetHum  = constrain(targetHum  + delta * 5, 30, 90); break;
        case 3: acPowerOn = !acPowerOn; if (acPowerOn) lastAcToggleTime = millis(); break;
        case 4: acCoolMode = !acCoolMode; break;
        case 5: humPowerOn = !humPowerOn; break;
        case 6: humiAddMode = !humiAddMode; break;
    }
}

// ===== 其他核心函数（传感器、控制、输出） =====
void readSensorData() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
        sensorError = true;
    } else {
        sensorError = false;
        currentTemp = t;
        currentHum = h;
    }
}

bool canToggleAC() {
    return millis() - lastAcToggleTime >= MIN_ON_OFF_TIME;
}

void controlRelays() {
    if (sensorError || !totalPowerOn) {
        acPowerOn = humPowerOn = false;
        return;
    }
    if (!autoMode) return;  // 手动模式下不自动控制

    const float tHys = 0.8;
    const float hHys = 6.0;

    humPowerOn = true;

    if (currentTemp > targetTemp + tHys) {
        acCoolMode = true;
        if (!acPowerOn && canToggleAC()) { acPowerOn = true; lastAcToggleTime = millis(); }
    } else if (currentTemp < targetTemp - tHys) {
        acCoolMode = false;
        if (!acPowerOn && canToggleAC()) { acPowerOn = true; lastAcToggleTime = millis(); }
    } else if (acPowerOn && canToggleAC()) {
        acPowerOn = false; lastAcToggleTime = millis();
    }

    if (currentHum > targetHum + hHys) humiAddMode = false;
    else if (currentHum < targetHum - hHys) humiAddMode = true;
}

void applyRelayOutputs() {
    digitalWrite(RELAY_TOTAL_POWER, totalPowerOn ? RELAY_ON : RELAY_OFF);
    if (!totalPowerOn) {
        digitalWrite(RELAY_AC_POWER, RELAY_OFF);
        digitalWrite(RELAY_AC_MODE, RELAY_OFF);
        digitalWrite(RELAY_HUM_POWER, RELAY_OFF);
        digitalWrite(RELAY_HUM_MODE, RELAY_OFF);
        return;
    }
    digitalWrite(RELAY_AC_POWER, acPowerOn ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY_AC_MODE, acPowerOn && acCoolMode ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY_HUM_POWER, humPowerOn ? RELAY_ON : RELAY_OFF);
    digitalWrite(RELAY_HUM_MODE, humPowerOn && humiAddMode ? RELAY_ON : RELAY_OFF);
}

// ===== 阈值报警 + LED 控制 =====
void checkAlarmAndLed() {
    if (sensorError) {
        // 传感器故障闪烁
        digitalWrite(LED_ALARM_PIN, (millis() % 600 < 300) ? HIGH : LOW);
        return;
    }

    bool alarm = false;

    // 温度：偏离目标 ±4.0℃ 才报警（容忍更大波动）
    // 或者用迟滞方式：一旦报警，要回到 ±2.5℃ 内才解除
    static bool tempAlarmActive = false;
    if (tempAlarmActive) {
        // 已报警状态：要求回到更小的偏差才解除
        if (currentTemp > targetTemp - 2.5 && currentTemp < targetTemp + 2.5) {
            tempAlarmActive = false;
        }
    } else {
        if (currentTemp > targetTemp + 4.0 || currentTemp < targetTemp - 4.0) {
            tempAlarmActive = true;
        }
    }
    if (tempAlarmActive) alarm = true;

    // 湿度同理
    static bool humAlarmActive = false;
    if (humAlarmActive) {
        if (currentHum > targetHum - 10.0 && currentHum < targetHum + 10.0) {
            humAlarmActive = false;
        }
    } else {
        if (currentHum > targetHum + 20.0 || currentHum < targetHum - 20.0) {
            humAlarmActive = true;
        }
    }
    if (humAlarmActive) alarm = true;

    digitalWrite(LED_ALARM_PIN, alarm ? HIGH : LOW);
}