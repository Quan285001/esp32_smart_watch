#pragma once
#include <Arduino.h>

// Thêm SCREEN_SLEEP vào danh sách trạng thái
enum ScreenState { SCREEN_WEATHER, SCREEN_ANALOG, SCREEN_MENU, SCREEN_SLEEP, SCREEN_INFO };
struct HourlyForecast {
    int hour;
    int temp;
    int weatherCode;
};

struct WeatherData {
    int currentTemp;
    int maxTemp;
    int minTemp;
    String conditionText;
    HourlyForecast hourly[12];
    int batteryPct;
    bool isCharging;
};