#pragma once
#include <Arduino.h>

enum ScreenState { SCREEN_BOOT, SCREEN_WEATHER, SCREEN_ANALOG, SCREEN_MENU, SCREEN_SLEEP, SCREEN_INFO };

struct HourlyForecast {
    uint32_t timestamp;
    int16_t temperature10;
    uint8_t precipitationProbability;
    uint16_t precipitation10;
    uint16_t windSpeed10;
    uint8_t uvIndex;
    uint8_t weatherCode;
};

struct WeatherData {
    int16_t currentTemp10;
    int16_t feelsLike10;
    int16_t maxTemp10;
    int16_t minTemp10;
    uint8_t humidity;
    uint8_t uvIndex;
    uint8_t cloudCover;
    uint16_t windSpeed10;
    uint16_t windGust10;
    uint16_t windDirection;
    uint16_t pressure;
    uint16_t rain10;
    uint8_t precipitationProbability;
    uint8_t hourlyCount;
    HourlyForecast hourly[24];
    char sunrise[6];
    char sunset[6];
    char moonrise[6];
    char moonset[6];
    uint8_t moonPhase;
    char updatedAt[20];
    char conditionText[24];
    bool valid;
    bool fromCache;
};
