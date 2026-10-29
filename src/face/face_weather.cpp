#include <Arduino.h>
#include <TFT_eSPI.h>
#include "icons.h"
#include "config.h"
#include <math.h>
#include <time.h>

extern TFT_eSPI tft;

void drawWeatherFaceData(const WeatherData& w) {
    tft.fillScreen(0x0000); // Nền đen

    int centerX = 120;
    int centerY = 120;
    int radius = 98; // Bán kính vòng tròn 24 giờ

    // Vẽ khung viền tròn trang trí
    tft.drawCircle(centerX, centerY, radius + 18, 0x3186);
    tft.drawCircle(centerX, centerY, radius - 16, 0x3186);

    // Lấy giờ hệ thống thực tế (0 - 23)
    struct tm timeinfo;
    int currentHour = 12;
    if (getLocalTime(&timeinfo)) {
        currentHour = timeinfo.tm_hour;
    }

    // Vòng lặp vẽ 24 mốc giờ quanh viền tròn (mỗi giờ cách nhau 360/24 = 15 độ)
    for (int i = 0; i < 24; i++) {
        float angleDeg = i * 15.0f - 90.0f; // Bắt đầu từ đỉnh (12h)
        float angleRad = angleDeg * (3.14159f / 180.0f);

        int hourVal = w.hourly[i].hour;
        bool isCurrentHour = (hourVal == currentHour);

        int textX = centerX + (int)(radius * cos(angleRad));
        int textY = centerY + (int)(radius * sin(angleRad));

        // Giờ hiện tại: Nền trắng, chữ đen; Các giờ khác: Chữ xám mờ
        if (isCurrentHour) {
            tft.fillCircle(textX, textY, 11, 0xFFFF); // Nền trắng
            tft.setTextColor(0x0000, 0xFFFF);        // Chữ đen
        } else {
            tft.setTextColor(0x7BEF, 0x0000);        // Xám mờ
        }

        tft.setTextDatum(MC_DATUM);
        char hourStr[4];
        sprintf(hourStr, "%02d", hourVal);
        tft.drawString(hourStr, textX, textY, 1);

        // Vẽ icon thời tiết nhỏ phía trong vòng số giờ
        int iconX = centerX + (int)((radius - 26) * cos(angleRad));
        int iconY = centerY + (int)((radius - 26) * sin(angleRad));
        
        bool isNightHourly = (hourVal < 6 || hourVal >= 18);
        int code = w.hourly[i].weatherCode;

        if (code == 0) {
            if (isNightHourly) drawMiniCrescentMoon(tft, iconX, iconY, 0xFCE0);
            else drawMiniSun(tft, iconX, iconY, 0xFDE0);
        } else if (code >= 1 && code <= 3) {
            if (isNightHourly) drawMiniMoonCloud(tft, iconX, iconY, 0xFFFF);
            else drawMiniSunCloud(tft, iconX, iconY, 0xFFFF);
        } else if (code >= 51 && code <= 67) {
            drawMiniRain(tft, iconX, iconY, 0x001F);
        } else if (code >= 95) {
            drawMiniThunder(tft, iconX, iconY, 0xFDE0);
        } else {
            drawMiniSunCloud(tft, iconX, iconY, 0xFFFF);
        }
    }

    // --- KHU VỰC TRUNG TÂM ---
    tft.setTextColor(0xFFFF, 0x0000);
    char tempStr[10];
    sprintf(tempStr, "%d°", w.currentTemp);
    tft.drawString(tempStr, centerX, centerY - 14, 4);

    char hlStr[24];
    sprintf(hlStr, "H:%d° L:%d°", w.maxTemp, w.minTemp);
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString(hlStr, centerX, centerY + 10, 1);

    tft.setTextColor(0xFDE0, 0x0000);
    tft.drawString(w.conditionText.c_str(), centerX, centerY + 28, 1);
}