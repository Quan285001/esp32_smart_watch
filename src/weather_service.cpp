#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <time.h>
#include <math.h>
#include "config.h"

WeatherData currentWeather = {};
static const char *CACHE_PATH = "/weather.json";
static const uint32_t CACHE_MAX_AGE = 6UL * 60UL * 60UL;

static bool parseLocalIsoMinute(const char *text, struct tm &out) {
    if (!text || !*text) return false;
    int year, month, day, hour, minute;
    if (sscanf(text, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) != 5) return false;
    if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59) return false;
    out = {};
    out.tm_year = year - 1900; out.tm_mon = month - 1; out.tm_mday = day;
    out.tm_hour = hour; out.tm_min = minute; out.tm_isdst = -1;
    return mktime(&out) > 0;
}

static String weatherDescription(int code) {
    if (code == 0) return "Clear";
    if (code <= 3) return "Cloudy";
    if (code == 45 || code == 48) return "Fog";
    if (code <= 67) return "Rain";
    if (code <= 77) return "Snow";
    if (code <= 82) return "Showers";
    if (code >= 95) return "Thunderstorm";
    return "Weather";
}

static void copyTime(char *dst, size_t size, JsonVariantConst value) {
    const char *src = value | "";
    if (strlen(src) >= 16) {
        snprintf(dst, size, "%c%c:%c%c", src[11], src[12], src[14], src[15]);
    } else {
        snprintf(dst, size, "--:--");
    }
}

bool loadWeatherCache() {
    if (!LittleFS.begin(false)) return false;
    File file = LittleFS.open(CACHE_PATH, "r");
    if (!file) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) return false;

    WeatherData next = {};
    next.currentTemp10 = doc["t"] | 0;
    next.feelsLike10 = doc["f"] | 0;
    next.maxTemp10 = doc["hi"] | 0;
    next.minTemp10 = doc["lo"] | 0;
    next.humidity = doc["h"] | 0;
    next.uvIndex = doc["uv"] | 0;
    next.cloudCover = doc["cc"] | 0;
    next.windSpeed10 = doc["ws"] | 0;
    next.windGust10 = doc["wg"] | 0;
    next.windDirection = doc["wd"] | 0;
    next.pressure = doc["p"] | 0;
    next.rain10 = doc["r"] | 0;
    next.precipitationProbability = doc["pp"] | 0;
    strlcpy(next.sunrise, doc["sr"] | "--:--", sizeof(next.sunrise));
    strlcpy(next.sunset, doc["ss"] | "--:--", sizeof(next.sunset));
    strlcpy(next.moonrise, doc["mr"] | "--:--", sizeof(next.moonrise));
    strlcpy(next.moonset, doc["ms"] | "--:--", sizeof(next.moonset));
    next.moonPhase = doc["mp"] | 0;
    strlcpy(next.updatedAt, doc["u"] | "", sizeof(next.updatedAt));
    strlcpy(next.conditionText, doc["c"] | "Cached", sizeof(next.conditionText));

    JsonArrayConst hours = doc["hours"].as<JsonArrayConst>();
    for (JsonObjectConst h : hours) {
        if (next.hourlyCount >= 24) break;
        HourlyForecast &out = next.hourly[next.hourlyCount++];
        out.timestamp = h["ts"] | 0;
        out.temperature10 = h["t"] | 0;
        out.precipitationProbability = h["pp"] | 0;
        out.precipitation10 = h["r"] | 0;
        out.windSpeed10 = h["w"] | 0;
        out.uvIndex = h["uv"] | 0;
        out.weatherCode = h["c"] | 0;
    }
    next.valid = true;
    next.fromCache = true;
    currentWeather = next;
    return true;
}

static bool saveWeatherCache(const WeatherData &w) {
    if (!LittleFS.begin(true)) return false;
    JsonDocument doc;
    doc["t"] = w.currentTemp10; doc["f"] = w.feelsLike10;
    doc["hi"] = w.maxTemp10; doc["lo"] = w.minTemp10;
    doc["h"] = w.humidity; doc["uv"] = w.uvIndex;
    doc["cc"] = w.cloudCover; doc["ws"] = w.windSpeed10;
    doc["wg"] = w.windGust10; doc["wd"] = w.windDirection;
    doc["p"] = w.pressure; doc["r"] = w.rain10;
    doc["pp"] = w.precipitationProbability;
    doc["sr"] = w.sunrise; doc["ss"] = w.sunset;
    doc["mr"] = w.moonrise; doc["ms"] = w.moonset; doc["mp"] = w.moonPhase;
    doc["u"] = w.updatedAt; doc["c"] = w.conditionText;
    JsonArray hours = doc["hours"].to<JsonArray>();
    for (uint8_t i = 0; i < w.hourlyCount; ++i) {
        JsonObject h = hours.add<JsonObject>();
        h["ts"] = w.hourly[i].timestamp; h["t"] = w.hourly[i].temperature10;
        h["pp"] = w.hourly[i].precipitationProbability;
        h["r"] = w.hourly[i].precipitation10; h["w"] = w.hourly[i].windSpeed10;
        h["uv"] = w.hourly[i].uvIndex; h["c"] = w.hourly[i].weatherCode;
    }
    File file = LittleFS.open(CACHE_PATH, "w");
    if (!file) return false;
    bool ok = serializeJson(doc, file) > 0;
    file.close();
    return ok;
}

bool weatherCacheIsStale(uint32_t maxAgeSeconds) {
    if (!currentWeather.valid || !currentWeather.updatedAt[0]) return true;
    struct tm updated = {};
    if (!parseLocalIsoMinute(currentWeather.updatedAt, updated)) return true;
    time_t then = mktime(&updated);
    time_t now = time(nullptr);
    if (then <= 0 || now <= then) return true;
    return (uint32_t)(now - then) > maxAgeSeconds;
}

bool fetchHourlyWeather() {
    if (WiFi.status() != WL_CONNECTED) return false;

    Preferences prefs;
    prefs.begin("wifi_store", true);
    float lat = prefs.getString("saved_lat", "10.823").toFloat();
    float lon = prefs.getString("saved_lon", "106.629").toFloat();
    prefs.end();
    if (lat < -90 || lat > 90 || lon < -180 || lon > 180) return false;

    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
        "&longitude=" + String(lon, 4) +
        "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,weather_code,cloud_cover,pressure_msl,wind_speed_10m,wind_direction_10m,wind_gusts_10m,uv_index" +
        "&hourly=temperature_2m,precipitation_probability,precipitation,wind_speed_10m,uv_index,weather_code" +
        "&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset,moonrise,moonset,moon_phase,precipitation_probability_max,precipitation_sum" +
        "&forecast_days=2&timezone=Asia%2FBangkok";

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(9000);
    if (!http.begin(client, url)) return false;
    int status = http.GET();
    if (status != HTTP_CODE_OK) { http.end(); return false; }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getStream());
    http.end();
    if (error || doc["current"].isNull()) return false;

    WeatherData next = {};
    JsonObjectConst cur = doc["current"];
    next.currentTemp10 = lroundf((cur["temperature_2m"] | 0.0f) * 10);
    next.feelsLike10 = lroundf((cur["apparent_temperature"] | 0.0f) * 10);
    next.humidity = cur["relative_humidity_2m"] | 0;
    next.uvIndex = (uint8_t)lroundf(cur["uv_index"] | 0.0f);
    next.cloudCover = cur["cloud_cover"] | 0;
    next.windSpeed10 = lroundf((cur["wind_speed_10m"] | 0.0f) * 10);
    next.windGust10 = lroundf((cur["wind_gusts_10m"] | 0.0f) * 10);
    next.windDirection = cur["wind_direction_10m"] | 0;
    next.pressure = lroundf(cur["pressure_msl"] | 0.0f);
    next.rain10 = lroundf((cur["rain"] | 0.0f) * 10);
    next.precipitationProbability = 0;
    strlcpy(next.conditionText, weatherDescription(cur["weather_code"] | 0).c_str(), sizeof(next.conditionText));

    JsonObjectConst daily = doc["daily"];
    next.maxTemp10 = lroundf((daily["temperature_2m_max"][0] | 0.0f) * 10);
    next.minTemp10 = lroundf((daily["temperature_2m_min"][0] | 0.0f) * 10);
    copyTime(next.sunrise, sizeof(next.sunrise), daily["sunrise"][0]);
    copyTime(next.sunset, sizeof(next.sunset), daily["sunset"][0]);
    copyTime(next.moonrise, sizeof(next.moonrise), daily["moonrise"][0]);
    copyTime(next.moonset, sizeof(next.moonset), daily["moonset"][0]);
    next.moonPhase = (uint8_t)lroundf((daily["moon_phase"][0] | 0.0f) * 100.0f);

    JsonArrayConst times = doc["hourly"]["time"];
    JsonArrayConst temps = doc["hourly"]["temperature_2m"];
    JsonArrayConst probs = doc["hourly"]["precipitation_probability"];
    JsonArrayConst rain = doc["hourly"]["precipitation"];
    JsonArrayConst wind = doc["hourly"]["wind_speed_10m"];
    JsonArrayConst uv = doc["hourly"]["uv_index"];
    JsonArrayConst codes = doc["hourly"]["weather_code"];
    time_t now = time(nullptr);
    for (size_t i = 0; i < times.size() && next.hourlyCount < 24; ++i) {
        const char *stamp = times[i] | "";
        struct tm tmHour = {};
        if (!parseLocalIsoMinute(stamp, tmHour)) continue;
        time_t epoch = mktime(&tmHour);
        if (epoch < now - 3600) continue;
        HourlyForecast &h = next.hourly[next.hourlyCount++];
        h.timestamp = epoch;
        h.temperature10 = lroundf((temps[i] | 0.0f) * 10);
        h.precipitationProbability = probs[i] | 0;
        h.precipitation10 = lroundf((rain[i] | 0.0f) * 10);
        h.windSpeed10 = lroundf((wind[i] | 0.0f) * 10);
        h.uvIndex = (uint8_t)lroundf(uv[i] | 0.0f);
        h.weatherCode = codes[i] | 0;
    }
    if (next.hourlyCount) next.precipitationProbability = next.hourly[0].precipitationProbability;

    time_t updated = time(nullptr);
    struct tm local;
    localtime_r(&updated, &local);
    strftime(next.updatedAt, sizeof(next.updatedAt), "%Y-%m-%dT%H:%M", &local);
    next.valid = true;
    next.fromCache = false;
    currentWeather = next;
    saveWeatherCache(currentWeather);
    return true;
}
