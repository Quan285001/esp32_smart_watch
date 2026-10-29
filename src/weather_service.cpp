#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Biến toàn cục lưu thời tiết
WeatherData currentWeather = {
    .currentTemp = 28, .maxTemp = 32, .minTemp = 24,
    .conditionText = "Mostly Cloudy",
    .hourly = {
        {12, 28, 0}, {1, 29, 1}, {2, 30, 2}, {3, 31, 0},
        {4, 30, 1},  {5, 29, 3}, {6, 28, 51},{7, 27, 0},
        {8, 26, 0},  {9, 25, 0}, {10, 25, 1},{11, 24, 0}
    },
    .batteryPct = 88, .isCharging = false
};

extern ScreenState currentScreen;
extern void drawWeatherFaceData(const WeatherData& w);

void fetchHourlyWeather() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = "https://api.open-meteo.com/v1/forecast?latitude=10.823&longitude=106.629&current=temperature_2m,weather_code&hourly=temperature_2m,weather_code&daily=temperature_2m_max,temperature_2m_min&timezone=Asia%2FBangkok";
        
        http.begin(url);
        int httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                currentWeather.currentTemp = doc["current"]["temperature_2m"].as<int>();
                currentWeather.maxTemp = doc["daily"]["temperature_2m_max"][0].as<int>();
                currentWeather.minTemp = doc["daily"]["temperature_2m_min"][0].as<int>();

                JsonArray hourlyTimes = doc["hourly"]["time"];
                JsonArray hourlyTemps = doc["hourly"]["temperature_2m"];
                JsonArray hourlyCodes = doc["hourly"]["weather_code"];

                for (int i = 0; i < 12; i++) {
                    String timeStr = hourlyTimes[i].as<String>();
                    int h = timeStr.substring(11, 13).toInt();
                    int h12 = h % 12;
                    if (h12 == 0) h12 = 12;

                    currentWeather.hourly[i].hour = h12;
                    currentWeather.hourly[i].temp = hourlyTemps[i].as<int>();
                    currentWeather.hourly[i].weatherCode = hourlyCodes[i].as<int>();
                }
            }
        }
        http.end();
    }
}