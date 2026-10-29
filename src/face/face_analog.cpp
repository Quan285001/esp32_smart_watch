#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <time.h>
#include "icons.h" // Nhập kho icon chung (đã bao gồm trăng, mây, mặt trời)

extern TFT_eSPI tft;

// Khai báo Sprite đệm 240x240 vẽ ngầm chống chớp nháy
TFT_eSprite spr = TFT_eSprite(&tft);
bool spriteInitialized = false;

#define COLOR_BG        0x0000  // Đen tuyền
#define COLOR_INNER_BG  0x18C3  // Xám đen vòng tròn trong
#define COLOR_HAND      0xFFFF  // Trắng sáng cho kim giờ/phút
#define COLOR_SEC_HAND  0xFBE0  // Cam rực cho kim giây
#define COLOR_TEXT_MAIN 0xFFFF  // Trắng
#define COLOR_TEXT_ACCENT 0xFBE0// Vàng cam nhạt

// Hàm vẽ icon thông minh theo buổi (Ngày / Đêm dựa theo giờ hệ thống)
void drawSmartWeatherIcon(int currentHour, int x, int y) {
    // Xác định ban đêm từ 18:00 tối đến 06:00 sáng
    bool isNight = (currentHour < 6 || currentHour >= 18);

    if (isNight) {
        // Ban đêm: Vẽ trăng khuyết nghệ thuật
        drawMiniCrescentMoon(spr, x, y, COLOR_TEXT_ACCENT);
    } else {
        // Ban ngày: Vẽ mặt trời rực rỡ
        drawMiniSun(spr, x, y, COLOR_TEXT_ACCENT);
    }
}

void drawAnalogClockFace(struct tm timeinfo, int millisValue, int heartRate, int steps) {
    (void)heartRate;
    (void)steps;

    if (!spriteInitialized) {
        spr.createSprite(240, 240);
        spriteInitialized = true;
    }

    spr.fillSprite(COLOR_BG);

    int cx = 120;
    int cy = 120;

    // 1. Vẽ vòng tròn phông nền ở trung tâm
    spr.fillCircle(cx, cy, 78, COLOR_INNER_BG);

    // 2. WIDGET TỐI GIẢN
    // Bên trái (9h): Ký hiệu khu vực (HCM)
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(COLOR_TEXT_ACCENT, COLOR_INNER_BG);
    spr.drawString("LOC", cx - 45, cy - 10, 1);
    spr.setTextColor(COLOR_TEXT_MAIN, COLOR_INNER_BG);
    spr.drawString("HCM", cx - 45, cy + 6, 2);

    // Bên phải (3h): Ngày trong tháng (Số to rõ)
    const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    spr.setTextColor(COLOR_TEXT_ACCENT, COLOR_INNER_BG);
    spr.drawString(days[timeinfo.tm_wday], cx + 45, cy - 10, 1);
    spr.setTextColor(COLOR_TEXT_MAIN, COLOR_INNER_BG);
    spr.drawNumber(timeinfo.tm_mday, cx + 45, cy + 6, 2);

    // Phía dưới (6h): Tự động đổi Icon theo giờ (Ngày: Mặt trời / Đêm: Trăng khuyết)
    drawSmartWeatherIcon(timeinfo.tm_hour, cx, cy + 38);
    spr.setTextColor(COLOR_TEXT_ACCENT, COLOR_INNER_BG);
    spr.drawString("WEATHER", cx, cy + 50, 1);

    // 3. TÍNH TOÁN GÓC QUAY MƯỢT MÀ (CÓ MILI-GIÂY)
    float exactSeconds = timeinfo.tm_sec + (millisValue / 1000.0f);
    float exactMinutes = timeinfo.tm_min + (exactSeconds / 60.0f);
    float exactHours   = (timeinfo.tm_hour % 12) + (exactMinutes / 60.0f);

    float angleHour = (exactHours * 30.0f - 90.0f) * (PI / 180.0f);
    float angleMin  = (exactMinutes * 6.0f - 90.0f) * (PI / 180.0f);
    float angleSec  = (exactSeconds * 6.0f - 90.0f) * (PI / 180.0f);

    // 4. VẼ KIM MẬP 3 PIXEL (Dày dặn, sắc nét)
    // Kim Giờ (Ngắn, mập dày 3 px)
    int hx = cx + (int)(40 * cos(angleHour));
    int hy = cy + (int)(40 * sin(angleHour));
    for (int offset = -1; offset <= 1; offset++) {
        spr.drawLine(cx + offset, cy, hx + offset, hy, COLOR_HAND);
        spr.drawLine(cx, cy + offset, hx, hy + offset, COLOR_HAND);
    }

    // Kim Phút (Dài, mập dày 3 px)
    int mx = cx + (int)(68 * cos(angleMin));
    int my = cy + (int)(68 * sin(angleMin));
    for (int offset = -1; offset <= 1; offset++) {
        spr.drawLine(cx + offset, cy, mx + offset, my, COLOR_HAND);
        spr.drawLine(cx, cy + offset, mx, my + offset, COLOR_HAND);
    }

    // Kim Giây (Màu cam mảnh mượt mà bản 2 px)
    int sx = cx + (int)(90 * cos(angleSec));
    int sy = cy + (int)(90 * sin(angleSec));
    int sx_back = cx - (int)(18 * cos(angleSec));
    int sy_back = cy - (int)(18 * sin(angleSec));
    spr.drawLine(sx_back, sy_back, sx, sy, COLOR_SEC_HAND);
    spr.drawLine(sx_back + 1, sy_back, sx + 1, sy, COLOR_SEC_HAND);

    // Tâm kim kép nổi bật
    spr.fillCircle(cx, cy, 5, COLOR_SEC_HAND);
    spr.fillCircle(cx, cy, 2, COLOR_TEXT_MAIN);

    // Đẩy khung hình ra màn hình phần cứng
    spr.pushSprite(0, 0);
}