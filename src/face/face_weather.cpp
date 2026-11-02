#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "config.h"

extern TFT_eSPI tft;
static const uint16_t BG=0x0000, WHITE=0xFFFF, MUTED=0x9CF3, BLUE=0x5DDF, YELLOW=0xFDE0, GREEN=0x07E0, RED=0xF800;

static void header(const char *title, const WeatherData &w, uint8_t page) {
    tft.fillScreen(BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(WHITE, BG);
    tft.drawString(title, 120, 20, 2);
    tft.drawFastHLine(24, 34, 192, 0x39E7);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(w.fromCache ? YELLOW : GREEN, BG);
    tft.drawString(w.valid ? (w.fromCache ? "CACHE" : "LIVE") : "NO DATA", 215, 47, 1);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(MUTED, BG);
    tft.drawString(w.updatedAt[0] ? w.updatedAt : "Not synchronized", 25, 226, 1);
    tft.setTextDatum(MC_DATUM);
    char pageText[8]; snprintf(pageText,sizeof(pageText),"%u/3",page);
    tft.drawString(pageText,120,226,1);
}
static void labelValue(int y,const char *label,const char *value,uint16_t color=WHITE) {
    tft.setTextDatum(ML_DATUM); tft.setTextColor(MUTED,BG); tft.drawString(label,28,y,1);
    tft.setTextDatum(MR_DATUM); tft.setTextColor(color,BG); tft.drawString(value,212,y,2);
}

void drawWeatherFaceData(const WeatherData &w, uint8_t page) {
    char value[24];
    if (page == 0) {
        header("CURRENT WEATHER",w,1);
        snprintf(value,sizeof(value),"%.1f C",w.currentTemp10/10.0f);
        tft.setTextDatum(MC_DATUM); tft.setTextColor(WHITE,BG); tft.drawString(value,120,88,4);
        tft.setTextColor(BLUE,BG); tft.drawString(w.conditionText,120,113,2);
        snprintf(value,sizeof(value),"Feels %.1f C",w.feelsLike10/10.0f);
        tft.setTextColor(MUTED,BG); tft.drawString(value,120,130,1);
        snprintf(value,sizeof(value),"H %.1f / L %.1f C",w.maxTemp10/10.0f,w.minTemp10/10.0f);
        tft.drawString(value,120,146,1);
        snprintf(value,sizeof(value),"%u%%",w.humidity); labelValue(166,"HUMIDITY",value);
        snprintf(value,sizeof(value),"%.1f km/h",w.windSpeed10/10.0f); labelValue(185,"WIND",value);
        snprintf(value,sizeof(value),"%u deg",w.windDirection); labelValue(204,"DIRECTION",value);
        snprintf(value,sizeof(value),"%u",w.uvIndex); labelValue(220,"UV INDEX",value,w.uvIndex>=6?RED:YELLOW);
    } else if (page == 1) {
        header("RAIN FORECAST",w,2);
        snprintf(value,sizeof(value),"%u%%",w.precipitationProbability);
        tft.setTextDatum(MC_DATUM); tft.setTextColor(BLUE,BG); tft.drawString(value,120,65,4);
        tft.setTextColor(MUTED,BG); tft.drawString("NEXT HOURS",120,91,1);
        for (uint8_t i=0;i<6 && i<w.hourlyCount;i++) {
            const HourlyForecast &h=w.hourly[i];
            int y=111+i*17;
            time_t ts=h.timestamp; struct tm tmv; localtime_r(&ts,&tmv);
            char hour[8]; strftime(hour,sizeof(hour),"%H:%M",&tmv);
            tft.setTextDatum(ML_DATUM); tft.setTextColor(WHITE,BG); tft.drawString(hour,27,y,1);
            tft.drawRect(75,y-5,92,7,0x4208);
            tft.fillRect(76,y-4,(int)(90*h.precipitationProbability/100.0f),5,BLUE);
            snprintf(value,sizeof(value),"%u%%",h.precipitationProbability);
            tft.setTextDatum(MR_DATUM); tft.setTextColor(WHITE,BG); tft.drawString(value,211,y,1);
        }
    } else {
        header("WIND & SUN",w,3);
        snprintf(value,sizeof(value),"%.1f km/h",w.windSpeed10/10.0f); labelValue(66,"WIND",value);
        snprintf(value,sizeof(value),"%.1f km/h",w.windGust10/10.0f); labelValue(91,"GUST",value);
        snprintf(value,sizeof(value),"%u deg",w.windDirection); labelValue(116,"DIRECTION",value);
        snprintf(value,sizeof(value),"%u",w.uvIndex); labelValue(141,"UV INDEX",value,w.uvIndex>=6?RED:YELLOW);
        snprintf(value,sizeof(value),"%u%%",w.cloudCover); labelValue(166,"CLOUD",value);
        snprintf(value,sizeof(value),"%u hPa",w.pressure); labelValue(160,"PRESSURE",value);
        labelValue(184,"SUNRISE",w.sunrise);
        labelValue(207,"SUNSET",w.sunset);
    }
}
