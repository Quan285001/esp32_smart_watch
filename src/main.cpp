#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include "config.h"

TFT_eSPI tft = TFT_eSPI();
BLECharacteristic *pCharacteristic;

// Khai báo đầy đủ các hàm từ các module khác
extern WeatherData currentWeather;
extern void fetchHourlyWeather();
extern void drawWeatherFaceData(const WeatherData& w);
extern void drawMenuScreen(int selectedIndex, bool wifiStatus, bool btStatus);
extern void drawSystemInfoScreen();
extern void drawAnalogClockFace(struct tm timeinfo, int millisValue = 0, int heartRate = 0, int steps = 0);
extern void showBootTerminal();

ScreenState currentScreen = SCREEN_ANALOG;

int menuIndex = 0;
bool wifiPowerStatus = true;
bool btPowerStatus = true;

const char* ssid = "Phuong Uyen";
const char* password = "123456uyen";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

#define BUTTON_PIN 9 
unsigned long pressStartTime = 0;
bool lastButtonState = HIGH;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Khai báo nguyên mẫu hàm refreshCurrentScreen
void refreshCurrentScreen();

// Hàm lưu Wi-Fi vào Flash vĩnh viễn
void saveWiFiToFlash(String ssid, String pass) {
    Preferences prefs;
    prefs.begin("wifi_store", false);
    prefs.putString("saved_ssid", ssid);
    prefs.putString("saved_pass", pass);
    prefs.end();
    Serial.println("Đã lưu Wi-Fi vào Flash: " + ssid);
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            String command = String(value.c_str());
            Serial.println("BLE Command: " + command);

            if (command == "SLEEP_ON") {
                currentScreen = SCREEN_SLEEP;
                refreshCurrentScreen();
            } 
            else if (command == "SLEEP_OFF") {
                currentScreen = SCREEN_ANALOG;
                refreshCurrentScreen();
            } 
            // 1. Đồng bộ thời gian thực từ Web / Điện thoại
            else if (command.startsWith("SYNC_TIME:")) {
                time_t epochTime = command.substring(10).toInt();
                struct timeval tv = { .tv_sec = epochTime, .tv_usec = 0 };
                settimeofday(&tv, NULL);
                Serial.println("Đã đồng bộ thời gian từ BLE thành công!");
            }
            // 2. Đồng bộ / Làm mới thời tiết theo yêu cầu
            else if (command == "SYNC_WEATHER") {
                fetchHourlyWeather();
                refreshCurrentScreen();
                Serial.println("Đã cập nhật lại thời tiết!");
            }
            // 3. Nhận và lưu tọa độ GPS mới, sau đó gọi load lại thời tiết
            else if (command.startsWith("GPS:")) {
                int commaIndex = command.indexOf(',', 4);
                if (commaIndex != -1) {
                    String latStr = command.substring(4, commaIndex);
                    String lonStr = command.substring(commaIndex + 1);

                    Preferences prefs;
                    prefs.begin("wifi_store", false);
                    prefs.putString("saved_lat", latStr);
                    prefs.putString("saved_lon", lonStr);
                    prefs.end();

                    Serial.println("Đã lưu GPS mới: " + latStr + ", " + lonStr);

                    fetchHourlyWeather();
                    refreshCurrentScreen();
                }
            }
            // 4. Quét Wi-Fi xung quanh gửi lên Web
            else if (command == "SCAN_WIFI") {
                WiFi.mode(WIFI_STA);
                WiFi.disconnect();
                int n = WiFi.scanNetworks();
                
                String jsonResult = "[";
                for (int i = 0; i < n; ++i) {
                    if (i > 0) jsonResult += ",";
                    jsonResult += "\"" + WiFi.SSID(i) + "\"";
                }
                jsonResult += "]";
                WiFi.scanDelete();

                pCharacteristic->setValue(jsonResult.c_str());
                pCharacteristic->notify();
                Serial.println("Đã gửi danh sách Wi-Fi qua BLE: " + jsonResult);
            }
            // 5. Lưu thông tin Wi-Fi vào Flash
            else if (command.startsWith("WIFI:")) {
                int commaIndex = command.indexOf(',', 5);
                if (commaIndex != -1) {
                    String newSsid = command.substring(5, commaIndex);
                    String newPass = command.substring(commaIndex + 1);

                    saveWiFiToFlash(newSsid, newPass);

                    WiFi.begin(newSsid.c_str(), newPass.c_str());
                    wifiPowerStatus = true;
                }
            }
        }
    }
};

void setupBLE() {
    BLEDevice::init("ESP32_Watch");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
                      
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902()); 
    
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
}

void refreshCurrentScreen() {
    if (currentScreen == SCREEN_SLEEP) {
        tft.fillScreen(0x0000);
        tft.setTextColor(0x4208, 0x0000);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Zzz...", 120, 120, 2);
        return;
    }

    struct tm timeinfo;
    bool hasTime = getLocalTime(&timeinfo);
    if (!hasTime) {
        timeinfo.tm_hour = 10; timeinfo.tm_min = 9; timeinfo.tm_sec = 30;
        timeinfo.tm_wday = 6; timeinfo.tm_mday = 28;
    }

    if (currentScreen == SCREEN_ANALOG) {
        tft.fillScreen(0x0000); 
        drawAnalogClockFace(timeinfo, 0, 92, 3457);
    } else if (currentScreen == SCREEN_WEATHER) {
        drawWeatherFaceData(currentWeather);
    } else if (currentScreen == SCREEN_MENU) {
        drawMenuScreen(menuIndex, wifiPowerStatus, btPowerStatus);
    } else if (currentScreen == SCREEN_INFO) {
        drawSystemInfoScreen();
    }
}

void handleButtonPress() {
    bool currentState = digitalRead(BUTTON_PIN);
    static bool actionTriggered = false; 

    if (currentScreen == SCREEN_SLEEP) {
        if (lastButtonState == HIGH && currentState == LOW) {
            delay(30);
            currentScreen = SCREEN_ANALOG; 
            refreshCurrentScreen();
        }
        lastButtonState = currentState;
        return;
    }

    if (lastButtonState == HIGH && currentState == LOW) {
        pressStartTime = millis();
        actionTriggered = false;
        delay(30); 
    } 
    else if (lastButtonState == LOW && currentState == LOW) {
        unsigned long holdDuration = millis() - pressStartTime;
        
        if (holdDuration >= 800 && !actionTriggered) {
            if (currentScreen != SCREEN_MENU) {
                currentScreen = SCREEN_MENU;
                menuIndex = 0;
            } else {
                if (menuIndex == 0) { 
                    currentScreen = SCREEN_ANALOG;
                } else if (menuIndex == 1) { 
                    wifiPowerStatus = !wifiPowerStatus;
                    if (wifiPowerStatus) {
                        WiFi.begin(ssid, password);
                    } else {
                        WiFi.disconnect(true);
                        WiFi.mode(WIFI_OFF);
                    }
                } else if (menuIndex == 2) { 
                    btPowerStatus = !btPowerStatus;
                    if (btPowerStatus) {
                        setupBLE();
                    } else {
                        BLEDevice::deinit(true);
                    }
                } else if (menuIndex == 3) { 
                    currentScreen = SCREEN_SLEEP; 
                    tft.fillScreen(0x0000); 
                } else if (menuIndex == 4) { 
                    currentScreen = SCREEN_INFO;
                }
            }
            refreshCurrentScreen();
            actionTriggered = true; 
        }
    } 
    else if (lastButtonState == LOW && currentState == HIGH) {
        unsigned long duration = millis() - pressStartTime;
        
        if (!actionTriggered && duration < 800) {
            if (currentScreen == SCREEN_INFO) {
                currentScreen = SCREEN_MENU; 
            } else if (currentScreen == SCREEN_MENU) {
                menuIndex = (menuIndex + 1) % 5; 
            } else {
                currentScreen = (currentScreen == SCREEN_ANALOG) ? SCREEN_WEATHER : SCREEN_ANALOG;
            }
            refreshCurrentScreen();
        }
        actionTriggered = false;
    }
    lastButtonState = currentState;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    tft.init();
    tft.setRotation(0);
    
    showBootTerminal();
    setupBLE();

    Preferences prefs;
    prefs.begin("wifi_store", true);
    String savedSsid = prefs.getString("saved_ssid", "");
    String savedPass = prefs.getString("saved_pass", "");
    prefs.end();

    if (savedSsid != "") {
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
    } else {
        WiFi.begin(ssid, password);
    }

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(300);
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        fetchHourlyWeather();
    }

    refreshCurrentScreen();
}

void loop() {
    handleButtonPress();

    if (currentScreen == SCREEN_ANALOG) {
        static int last_sec = -1;
        static struct tm cached_time;
        static unsigned long last_milli_base = 0;

        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            if (timeinfo.tm_sec != last_sec) {
                last_sec = timeinfo.tm_sec;
                cached_time = timeinfo;
                last_milli_base = millis();
            }
        } else {
            cached_time.tm_hour = 12;
            cached_time.tm_min = 0;
            cached_time.tm_sec = (millis() / 1000) % 60;
            cached_time.tm_wday = 1;
            cached_time.tm_mday = 28;
        }

        unsigned long elapsed = millis() - last_milli_base;
        if (elapsed > 1000) elapsed = 1000;

        drawAnalogClockFace(cached_time, elapsed, 92, 3457);
    }

    static unsigned long lastMillis = 0;
    if (wifiPowerStatus && WiFi.status() == WL_CONNECTED && millis() - lastMillis > 1800000) {
        fetchHourlyWeather();
        lastMillis = millis();
    }
    
    delay(30); 
}