#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <sys/time.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include "config.h"

TFT_eSPI tft = TFT_eSPI();
BLECharacteristic *pCharacteristic = nullptr;

extern WeatherData currentWeather;
extern bool fetchHourlyWeather();
extern bool loadWeatherCache();
extern bool weatherCacheIsStale(uint32_t maxAgeSeconds);
extern void drawWeatherFaceData(const WeatherData &w, uint8_t page);
extern void drawMenuScreen(int selectedIndex, bool wifiStatus, bool btStatus);
extern void drawSystemInfoScreen();
extern void drawAnalogClockFace(struct tm timeinfo, int millisValue = 0, int heartRate = 0, int steps = 0);
extern void showBootTerminal();

ScreenState currentScreen = SCREEN_ANALOG;
static uint8_t weatherPage = 0;
static int menuIndex = 0;
static bool wifiPowerStatus = true;
static bool btPowerStatus = true;

static const char *ntpServer = "pool.ntp.org";
static const long gmtOffset_sec = 7 * 3600;
static const int daylightOffset_sec = 0;
#define BUTTON_PIN 9
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

enum CommandType : uint8_t { CMD_NONE, CMD_SLEEP_ON, CMD_SLEEP_OFF, CMD_SYNC_TIME, CMD_SYNC_WEATHER, CMD_GPS, CMD_SCAN_WIFI, CMD_WIFI };
struct BleCommand { CommandType type; char payload[112]; };
static QueueHandle_t commandQueue = nullptr;
static volatile bool weatherFetchRunning = false;
static volatile bool weatherFetchFinished = false;
static volatile bool weatherFetchOk = false;

static void enqueueCommand(CommandType type, const String &payload = "") {
    if (!commandQueue) return;
    BleCommand cmd = {};
    cmd.type = type;
    strlcpy(cmd.payload, payload.c_str(), sizeof(cmd.payload));
    // Drop the oldest command if the queue is full; BLE callbacks must never block.
    if (xQueueSend(commandQueue, &cmd, 0) != pdTRUE) {
        BleCommand discarded;
        xQueueReceive(commandQueue, &discarded, 0);
        xQueueSend(commandQueue, &cmd, 0);
    }
}

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) override {
        std::string value = characteristic->getValue();
        if (value.empty()) return;
        String command(value.c_str());
        if (command == "SLEEP_ON") enqueueCommand(CMD_SLEEP_ON);
        else if (command == "SLEEP_OFF") enqueueCommand(CMD_SLEEP_OFF);
        else if (command == "SYNC_WEATHER") enqueueCommand(CMD_SYNC_WEATHER);
        else if (command == "SCAN_WIFI") enqueueCommand(CMD_SCAN_WIFI);
        else if (command.startsWith("SYNC_TIME:")) enqueueCommand(CMD_SYNC_TIME, command.substring(10));
        else if (command.startsWith("GPS:")) enqueueCommand(CMD_GPS, command.substring(4));
        else if (command.startsWith("WIFI:")) enqueueCommand(CMD_WIFI, command.substring(5));
    }
};

static void setupBLE() {
    BLEDevice::init("ESP32_Watch");
    BLEServer *server = BLEDevice::createServer();
    BLEService *service = server->createService(SERVICE_UUID);
    pCharacteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());
    service->start();
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    BLEDevice::startAdvertising();
}

static void sendBleText(const String &message) {
    if (!pCharacteristic || !btPowerStatus) return;
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();
}

static void saveWiFi(const String &ssid, const String &pass) {
    Preferences prefs;
    prefs.begin("wifi_store", false);
    prefs.putString("saved_ssid", ssid);
    prefs.putString("saved_pass", pass);
    prefs.end();
}

void refreshCurrentScreen();

static void weatherFetchTask(void *parameter) {
    (void)parameter;
    weatherFetchOk = fetchHourlyWeather();
    weatherFetchFinished = true;
    weatherFetchRunning = false;
    vTaskDelete(nullptr);
}

static bool startWeatherFetch() {
    if (weatherFetchRunning) return false;
    weatherFetchRunning = true;
    weatherFetchFinished = false;
    if (xTaskCreate(weatherFetchTask, "weatherFetch", 8192, nullptr, 1, nullptr) != pdPASS) {
        weatherFetchRunning = false;
        return false;
    }
    return true;
}

static void processBleCommand(const BleCommand &cmd) {
    switch (cmd.type) {
    case CMD_SLEEP_ON:
        currentScreen = SCREEN_SLEEP;
        refreshCurrentScreen();
        break;
    case CMD_SLEEP_OFF:
        currentScreen = SCREEN_ANALOG;
        refreshCurrentScreen();
        break;
    case CMD_SYNC_TIME: {
        const long epoch = strtol(cmd.payload, nullptr, 10);
        if (epoch > 1700000000L) {
            struct timeval tv = { (time_t)epoch, 0 };
            settimeofday(&tv, nullptr);
            sendBleText("TIME_OK");
        } else sendBleText("TIME_INVALID");
        break;
    }
    case CMD_SYNC_WEATHER:
        sendBleText(startWeatherFetch() ? "WEATHER_QUEUED" : "WEATHER_BUSY");
        break;
    case CMD_GPS: {
        char *comma = strchr(cmd.payload, ',');
        if (!comma) { sendBleText("GPS_INVALID"); break; }
        *comma = '\0';
        const float lat = strtof(cmd.payload, nullptr);
        const float lon = strtof(comma + 1, nullptr);
        if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
            sendBleText("GPS_INVALID"); break;
        }
        Preferences prefs;
        prefs.begin("wifi_store", false);
        prefs.putString("saved_lat", String(lat, 6));
        prefs.putString("saved_lon", String(lon, 6));
        prefs.end();
        sendBleText(startWeatherFetch() ? "GPS_WEATHER_QUEUED" : "GPS_SAVED_WEATHER_PENDING");
        break;
    }
    case CMD_SCAN_WIFI: {
        if (WiFi.status() != WL_CONNECTED) WiFi.mode(WIFI_STA);
        int n = WiFi.scanNetworks(false, true);
        String result = "[";
        for (int i = 0; i < n; ++i) {
            if (i) result += ",";
            String name = WiFi.SSID(i);
            name.replace("\\", "\\\\"); name.replace("\"", "\\\"");
            result += "\"" + name + "\"";
        }
        result += "]";
        WiFi.scanDelete();
        sendBleText(result);
        break;
    }
    case CMD_WIFI: {
        char *comma = strchr(cmd.payload, ',');
        if (!comma) { sendBleText("WIFI_INVALID"); break; }
        *comma = '\0';
        String newSsid(cmd.payload), newPass(comma + 1);
        saveWiFi(newSsid, newPass);
        WiFi.mode(WIFI_STA);
        wifiPowerStatus = true;
        WiFi.begin(newSsid.c_str(), newPass.c_str());
        sendBleText("WIFI_CONNECTING");
        break;
    }
    default: break;
    }
}

void refreshCurrentScreen() {
    if (currentScreen == SCREEN_SLEEP) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(0x4208, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Zzz...", 120, 120, 2);
        return;
    }
    struct tm timeinfo = {};
    if (!getLocalTime(&timeinfo, 10)) {
        timeinfo.tm_hour = 10; timeinfo.tm_min = 9; timeinfo.tm_sec = 30;
        timeinfo.tm_wday = 6; timeinfo.tm_mday = 28;
    }
    if (currentScreen == SCREEN_ANALOG) {
        tft.fillScreen(TFT_BLACK);
        drawAnalogClockFace(timeinfo, 0, 0, 0);
    } else if (currentScreen == SCREEN_WEATHER) {
        drawWeatherFaceData(currentWeather, weatherPage);
    } else if (currentScreen == SCREEN_MENU) {
        drawMenuScreen(menuIndex, wifiPowerStatus, btPowerStatus);
    } else if (currentScreen == SCREEN_INFO) {
        drawSystemInfoScreen();
    }
}

static void handleButtonPress() {
    static bool stableState = HIGH, lastReading = HIGH;
    static uint32_t changedAt = 0, pressedAt = 0;
    static bool longActionDone = false;
    bool reading = digitalRead(BUTTON_PIN);
    uint32_t now = millis();
    if (reading != lastReading) { lastReading = reading; changedAt = now; }
    if (now - changedAt < 30 || reading == stableState) return;
    stableState = reading;

    if (stableState == LOW) {
        pressedAt = now;
        longActionDone = false;
        return;
    }

    uint32_t held = now - pressedAt;
    if (held >= 900) {
        longActionDone = true;
        if (currentScreen != SCREEN_MENU) {
            currentScreen = SCREEN_MENU;
            menuIndex = 0;
        } else {
            switch (menuIndex) {
            case 0: currentScreen = SCREEN_ANALOG; break;
            case 1:
                wifiPowerStatus = !wifiPowerStatus;
                if (wifiPowerStatus) {
                    Preferences p; p.begin("wifi_store", true);
                    String s = p.getString("saved_ssid", "");
                    String pw = p.getString("saved_pass", "");
                    p.end();
                    if (s.length()) WiFi.begin(s.c_str(), pw.c_str());
                } else { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); }
                break;
            case 2:
                // Keep the BLE host initialized; only stop/start advertising to avoid unsafe deinit cycles.
                btPowerStatus = !btPowerStatus;
                if (btPowerStatus) BLEDevice::startAdvertising();
                else BLEDevice::getAdvertising()->stop();
                break;
            case 3: currentScreen = SCREEN_SLEEP; break;
            case 4: currentScreen = SCREEN_INFO; break;
            }
        }
        refreshCurrentScreen();
    } else if (!longActionDone) {
        if (currentScreen == SCREEN_SLEEP) currentScreen = SCREEN_ANALOG;
        else if (currentScreen == SCREEN_INFO) currentScreen = SCREEN_MENU;
        else if (currentScreen == SCREEN_MENU) menuIndex = (menuIndex + 1) % 5;
        else if (currentScreen == SCREEN_WEATHER) weatherPage = (weatherPage + 1) % 3;
        else currentScreen = SCREEN_WEATHER;
        refreshCurrentScreen();
    }
}

void setup() {
    Serial.begin(115200);
    delay(250);
    commandQueue = xQueueCreate(6, sizeof(BleCommand));
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    setenv("TZ", "ICT-7", 1);
    tzset();
    tft.init();
    tft.setRotation(0);
    showBootTerminal();

    // Restore cached weather immediately so the watch remains useful offline.
    loadWeatherCache();
    setupBLE();

    Preferences prefs;
    prefs.begin("wifi_store", true);
    String savedSsid = prefs.getString("saved_ssid", "");
    String savedPass = prefs.getString("saved_pass", "");
    prefs.end();

    if (savedSsid.length()) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
    } else {
        wifiPowerStatus = false;
        WiFi.mode(WIFI_OFF);
    }

    if (savedSsid.length()) {
        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 7000) {
            delay(100);
            yield();
        }
        if (WiFi.status() == WL_CONNECTED) {
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        }
    }
    refreshCurrentScreen();
    if (WiFi.status() == WL_CONNECTED) startWeatherFetch();
}

void loop() {
    handleButtonPress();

    BleCommand cmd;
    if (commandQueue && xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
        processBleCommand(cmd);
    }

    if (weatherFetchFinished) {
        weatherFetchFinished = false;
        sendBleText(weatherFetchOk ? "WEATHER_OK" : "WEATHER_FAILED");
        if (currentScreen == SCREEN_WEATHER) refreshCurrentScreen();
    }

    static uint32_t lastWeatherCheck = 0;
    static uint32_t lastClockDraw = 0;
    static bool wasWifiConnected = false;
    uint32_t now = millis();
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    if (wifiConnected && !wasWifiConnected) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        lastWeatherCheck = now;
        startWeatherFetch();
    }
    wasWifiConnected = wifiConnected;
    if (wifiPowerStatus && wifiConnected && now - lastWeatherCheck >= 1800000UL) {
        lastWeatherCheck = now;
        if (weatherCacheIsStale(1800)) startWeatherFetch();
    }

    if (currentScreen == SCREEN_ANALOG && now - lastClockDraw >= 100) {
        lastClockDraw = now;
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5)) {
            uint16_t ms = (uint16_t)(now % 1000);
            drawAnalogClockFace(timeinfo, ms, 0, 0);
        }
    }
    delay(5);
}
