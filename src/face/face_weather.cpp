#include <Arduino.h>
#include <TFT_eSPI.h>
#include "icons.h"
#include "config.h"
#include <math.h>
#include <time.h>

extern TFT_eSPI tft;

void drawWeatherFaceData(const WeatherData& w) {
    tft.fillScreen(0x0000); // Xóa nền đen

    int centerX = 120;
    int centerY = 120;
    int radius = 105; // Bán kính vòng tròn hiển thị mốc giờ

    // Vẽ khung viền tròn đôi trang trí đậm chất smartwatch
    tft.drawCircle(centerX, centerY, radius + 14, 0x3186);
    tft.drawCircle(centerX, centerY, radius + 13, 0x3186);
    tft.drawCircle(centerX, centerY, radius - 14, 0x3186);
    tft.drawCircle(centerX, centerY, radius - 13, 0x3186);

    // Lấy giờ hệ thống để lọc trạng thái Ngày / Đêm cho icon thời tiết từng giờ
    struct tm timeinfo;
    int currentHour = 12; // Mặc định ban ngày nếu chưa lấy được giờ
    if (getLocalTime(&timeinfo)) {
        currentHour = timeinfo.tm_hour;
    }

    // Vòng lặp vẽ 12 mốc giờ và icon thời tiết phân bổ quanh viền tròn
    for (int i = 0; i < 12; i++) {
        float angle = (i * 30.0 - 90.0) * (3.14159 / 180.0);

        int textX = centerX + (int)(radius * cos(angle));
        int textY = centerY + (int)(radius * sin(angle));

        int hourVal = w.hourly[i].hour;
        
        // Vẽ số giờ
        tft.setTextColor(0xCE79, 0x0000);
        tft.setTextDatum(MC_DATUM);
        tft.drawNumber(hourVal, textX, textY, 2);

        // Tọa độ vẽ icon phía trong vòng số giờ
        int iconX = centerX + (int)((radius - 28) * cos(angle));
        int iconY = centerY + (int)((radius - 28) * sin(angle));
        
        // Xác định giờ của mốc dự báo này là ngày hay đêm
        bool isNightHourly = (hourVal < 6 || hourVal >= 18);

        // Gọi icon tương ứng từ icons.h (Tự động đổi Trăng/Mặt trời cực kỳ thông minh)
        int code = w.hourly[i].weatherCode;
        if (code == 0) {
            if (isNightHourly) {
                drawMiniCrescentMoon(tft, iconX, iconY, 0xFCE0); // Trăng khuyết ban đêm
            } else {
                drawMiniSun(tft, iconX, iconY, 0xFDE0);          // Mặt trời ban ngày
            }
        } else if (code >= 1 && code <= 3) {
            if (isNightHourly) {
                drawMiniMoonCloud(tft, iconX, iconY, 0xFFFF);    // Trăng mây đêm
            } else {
                drawMiniSunCloud(tft, iconX, iconY, 0xFFFF);     // Nắng mây ngày
            }
        } else if (code >= 51 && code <= 67) {
            drawMiniRain(tft, iconX, iconY, 0x001F);             // Mưa
        } else if (code >= 95) {
            drawMiniThunder(tft, iconX, iconY, 0xFDE0);          // Giông bão
        } else {
            drawMiniSunCloud(tft, iconX, iconY, 0xFFFF);
        }
    }

    // --- KHU VỰC TRUNG TÂM (CHÍNH GIỮA MẶT ĐỒNG HỒ) ---
    // 1. Nhiệt độ hiện tại (Số cực lớn)
    tft.setTextColor(0xFFFF, 0x0000);
    char tempStr[10];
    sprintf(tempStr, "%d°", w.currentTemp);
    tft.drawString(tempStr, centerX + 12, centerY - 20, 6);

    // 2. Nhiệt độ Cao nhất / Thấp nhất trong ngày (H & L)
    char hlStr[24];
    sprintf(hlStr, "H:%d°  L:%d°", w.maxTemp, w.minTemp);
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString(hlStr, centerX + 13, centerY + 12, 2);

    // 3. Tình trạng thời tiết (Ví dụ: Clear sky, Rain,...)
    tft.setTextColor(0xFDE0, 0x0000);
    tft.drawString(w.conditionText.c_str(), centerX, centerY + 36, 2);
}