#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "config.h"

TFT_eSPI tft = TFT_eSPI();

// Khai báo đầy đủ các hàm từ các module khác để main.cpp nhận diện được
extern WeatherData currentWeather;
extern void fetchHourlyWeather();
extern void drawWeatherFaceData(const WeatherData& w);
extern void drawMenuScreen(int selectedIndex, bool wifiStatus);
extern void drawSystemInfoScreen(); // Màn hình System Info vừa thêm
extern void drawAnalogClockFace(struct tm timeinfo, int millisValue = 0, int heartRate = 0, int steps = 0);
ScreenState currentScreen = SCREEN_ANALOG;

int menuIndex = 0;
bool wifiPowerStatus = true;

const char* ssid = "Phuong Uyen";
const char* password = "123456uyen";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

#define BUTTON_PIN 9 
unsigned long pressStartTime = 0;
bool lastButtonState = HIGH;

// Trong hàm refreshCurrentScreen():
void refreshCurrentScreen() {
    if (currentScreen == SCREEN_SLEEP) {
        tft.fillScreen(0x0000);
        return;
    }

    struct tm timeinfo;
    bool hasTime = getLocalTime(&timeinfo);
    if (!hasTime) {
        timeinfo.tm_hour = 10; timeinfo.tm_min = 9; timeinfo.tm_sec = 30;
        timeinfo.tm_wday = 6; timeinfo.tm_mday = 28;
    }

    if (currentScreen == SCREEN_ANALOG) {
        drawAnalogClockFace(timeinfo, 0, 92, 3457);
    } else if (currentScreen == SCREEN_WEATHER) {
        drawWeatherFaceData(currentWeather);
    } else if (currentScreen == SCREEN_MENU) {
        drawMenuScreen(menuIndex, wifiPowerStatus);
    } else if (currentScreen == SCREEN_INFO) {
        drawSystemInfoScreen(); // Gọi màn hình thông tin hệ thống
    }
}

void handleButtonPress() {
    bool currentState = digitalRead(BUTTON_PIN);
    static bool actionTriggered = false; // Cờ đánh dấu đã thực hiện hành động giữ

    // TRƯỜNG HỢP ĐANG Ở CHẾ ĐỘ NGỦ: Bấm bất kỳ nút nào để thức giấc
    if (currentScreen == SCREEN_SLEEP) {
        if (lastButtonState == HIGH && currentState == LOW) {
            delay(30);
            currentScreen = SCREEN_ANALOG; // Thức dậy về mặt đồng hồ chính
            refreshCurrentScreen();
        }
        lastButtonState = currentState;
        return;
    }

    // Khi bắt đầu nhấn nút xuống
    if (lastButtonState == HIGH && currentState == LOW) {
        pressStartTime = millis();
        actionTriggered = false;
        delay(30); // Chống dội
    } 
    // Đang trong lúc giữ nút
    else if (lastButtonState == LOW && currentState == LOW) {
        unsigned long holdDuration = millis() - pressStartTime;
        
        // Nếu giữ quá 800ms mà chưa làm gì trong lần nhấn này
        if (holdDuration >= 800 && !actionTriggered) {
            if (currentScreen != SCREEN_MENU) {
                // Đang ở mặt đồng hồ -> Giữ để MỞ MENU
                currentScreen = SCREEN_MENU;
                menuIndex = 0;
            } else {
                // Đang ở trong Menu -> Giữ để CHỌN MỤC ĐANG SÁNG
                if (menuIndex == 0) { // 0: Bật/Tắt Wi-Fi
                    wifiPowerStatus = !wifiPowerStatus;
                    if (wifiPowerStatus) {
                        WiFi.begin(ssid, password);
                    } else {
                        WiFi.disconnect(true);
                        WiFi.mode(WIFI_OFF);
                    }
                } else if (menuIndex == 1) { // 1: Đổi mặt đồng hồ (Ví dụ chuyển về mặt Analog)
                    currentScreen = SCREEN_ANALOG;
                } else if (menuIndex == 2) { // 2: Bật chế độ Ngủ
                    currentScreen = SCREEN_SLEEP;
                } else if (menuIndex == 3) { // 3: Mở màn hình System Info
                    currentScreen = SCREEN_INFO;
                }
            }
            refreshCurrentScreen();
            actionTriggered = true; // Đánh dấu đã thực hiện, tránh lặp
        }
    } 
    // Khi thả tay ra khỏi nút
    else if (lastButtonState == LOW && currentState == HIGH) {
        unsigned long duration = millis() - pressStartTime;
        
        // Nếu chưa bị kích hoạt hành động giữ và thời gian bấm ngắn (< 800ms)
        if (!actionTriggered && duration < 800) {
            if (currentScreen == SCREEN_INFO) {
                currentScreen = SCREEN_MENU; // Bấm ngắn để quay lại menu cài đặt
            } else if (currentScreen == SCREEN_MENU) {
                menuIndex = (menuIndex + 1) % 4; // Đúng 4 mục (0, 1, 2, 3)
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
    tft.fillScreen(0x0000); // Xóa sạch màn hình tránh nhiễu tĩnh lúc khởi động

    WiFi.begin(ssid, password);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(500);
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

    // Nếu đang thức và ở mặt đồng hồ kim thì chạy kim mượt mà
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
        }

        unsigned long elapsed = millis() - last_milli_base;
        if (elapsed > 1000) elapsed = 1000;

        drawAnalogClockFace(cached_time, elapsed, 92, 3457);
    }

    // Tải ngầm thời tiết mỗi 30 phút nếu Wi-Fi bật
    static unsigned long lastMillis = 0;
    if (wifiPowerStatus && WiFi.status() == WL_CONNECTED && millis() - lastMillis > 1800000) {
        fetchHourlyWeather();
        lastMillis = millis();
    }
    
    delay(30); 
}