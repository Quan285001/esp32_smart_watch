#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <time.h>
#include "icons.h"

extern TFT_eSPI tft;

static TFT_eSprite sprFace = TFT_eSprite(&tft);
static bool sprFaceCreated = false;

#define COLOR_BG        0x0000
#define COLOR_INNER_BG  0x18C3
#define COLOR_HAND      0xFFFF
#define COLOR_SEC_HAND  0xFBE0
#define COLOR_TEXT_MAIN 0xFFFF
#define COLOR_TEXT_ACCENT 0xFBE0

void drawSmartWeatherIconSpr(int currentHour, int x, int y) {
    bool isNight = (currentHour < 6 || currentHour >= 18);
    if (isNight) {
        drawMiniCrescentMoon(sprFace, x, y, COLOR_TEXT_ACCENT);
    } else {
        drawMiniSun(sprFace, x, y, COLOR_TEXT_ACCENT);
    }
}

void drawAnalogClockFace(struct tm timeinfo, int millisValue, int heartRate, int steps) {
    (void)heartRate;
    (void)steps;

    // Đảm bảo khởi tạo an toàn
    if (!sprFaceCreated) {
        sprFace.setColorDepth(16);
        if (sprFace.createSprite(160, 160)) {
            sprFaceCreated = true;
        } else {
            return; // Nếu không đủ RAM thì thoát an toàn tránh crash
        }
    }

    sprFace.fillSprite(COLOR_BG);

    int cx = 80;
    int cy = 80;

    sprFace.fillCircle(cx, cy, 78, COLOR_INNER_BG);

    sprFace.setTextDatum(MC_DATUM);
    sprFace.setTextColor(COLOR_TEXT_ACCENT, COLOR_INNER_BG);
    sprFace.drawString("LOC", cx - 40, cy - 10, 1);
    sprFace.setTextColor(COLOR_TEXT_MAIN, COLOR_INNER_BG);
    sprFace.drawString("HCM", cx - 40, cy + 6, 2);

    const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    int wday = (timeinfo.tm_wday >= 0 && timeinfo.tm_wday < 7) ? timeinfo.tm_wday : 0;
    sprFace.setTextColor(COLOR_TEXT_ACCENT, COLOR_INNER_BG);
    sprFace.drawString(days[wday], cx + 40, cy - 10, 1);
    sprFace.setTextColor(COLOR_TEXT_MAIN, COLOR_INNER_BG);
    sprFace.drawNumber(timeinfo.tm_mday > 0 ? timeinfo.tm_mday : 1, cx + 40, cy + 6, 2);

    drawSmartWeatherIconSpr(timeinfo.tm_hour, cx, cy + 38);
    sprFace.setTextColor(COLOR_TEXT_ACCENT, COLOR_INNER_BG);
    sprFace.drawString("WEATHER", cx, cy + 50, 1);

    float exactSeconds = timeinfo.tm_sec + (millisValue / 1000.0f);
    float exactMinutes = timeinfo.tm_min + (exactSeconds / 60.0f);
    float exactHours   = (timeinfo.tm_hour % 12) + (exactMinutes / 60.0f);

    float angleHour = (exactHours * 30.0f - 90.0f) * (PI / 180.0f);
    float angleMin  = (exactMinutes * 6.0f - 90.0f) * (PI / 180.0f);
    float angleSec  = (exactSeconds * 6.0f - 90.0f) * (PI / 180.0f);

    int hx = cx + (int)(40 * cos(angleHour));
    int hy = cy + (int)(40 * sin(angleHour));
    for (int offset = -1; offset <= 1; offset++) {
        sprFace.drawLine(cx + offset, cy, hx + offset, hy, COLOR_HAND);
        sprFace.drawLine(cx, cy + offset, hx, hy + offset, COLOR_HAND);
    }

    int mx = cx + (int)(68 * cos(angleMin));
    int my = cy + (int)(68 * sin(angleMin));
    for (int offset = -1; offset <= 1; offset++) {
        sprFace.drawLine(cx + offset, cy, mx + offset, my, COLOR_HAND);
        sprFace.drawLine(cx, cy + offset, mx, my + offset, COLOR_HAND);
    }

    int sx = cx + (int)(90 * cos(angleSec));
    int sy = cy + (int)(90 * sin(angleSec));
    int sx_back = cx - (int)(18 * cos(angleSec));
    int sy_back = cy - (int)(18 * sin(angleSec));
    sprFace.drawLine(sx_back, sy_back, sx, sy, COLOR_SEC_HAND);
    sprFace.drawLine(sx_back + 1, sy_back, sx + 1, sy, COLOR_SEC_HAND);

    sprFace.fillCircle(cx, cy, 5, COLOR_SEC_HAND);
    sprFace.fillCircle(cx, cy, 2, COLOR_TEXT_MAIN);

    // Đẩy sprite ra màn hình chính tại tọa độ (40, 40)
    sprFace.pushSprite(40, 40);
}