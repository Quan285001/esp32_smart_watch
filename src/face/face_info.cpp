#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

extern TFT_eSPI tft;

void drawSystemInfoScreen() {
    tft.fillScreen(0x0000); // Nền đen

    int centerX = 120;

    // Tiêu đề
    tft.setTextColor(0xFFFF, 0x0000);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("SYSTEM INFO", centerX, 22, 2);
    tft.drawFastHLine(40, 36, 160, 0x3186);

    // Lấy thông số hệ thống
    unsigned long uptimeSeconds = millis() / 1000;
    int hours = uptimeSeconds / 3600;
    int minutes = (uptimeSeconds % 3600) / 60;
    int seconds = uptimeSeconds % 60;
    
    char uptimeStr[16];
    sprintf(uptimeStr, "%02d:%02d:%02d", hours, minutes, seconds);

    uint32_t freeHeap = ESP.getFreeHeap();
    String ipStr = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "Disconnected";

    // In thông số ra màn hình
    tft.setTextDatum(ML_DATUM);
    int startY = 60;
    int spacing = 28;

    // 1. Uptime
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString("Uptime:", 30, startY, 1);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.drawString(uptimeStr, 95, startY, 2);

    // 2. Free RAM (Heap)
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString("Free RAM:", 30, startY + spacing, 1);
    tft.setTextColor(0xFDE0, 0x0000);
    char heapStr[20];
    sprintf(heapStr, "%d KB", freeHeap / 1024);
    tft.drawString(heapStr, 95, startY + spacing, 2);

    // 3. Wi-Fi Status / IP
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString("IP Address:", 30, startY + (spacing * 2), 1);
    tft.setTextColor(0x07E0, 0x0000); // Xanh lá nếu có IP
    tft.drawString(ipStr.c_str(), 30, startY + (spacing * 2) + 14, 1);

    // Hướng dẫn quay lại
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0x7BEF, 0x0000);
    tft.drawString("[Press: Back to Menu]", centerX, 205, 1);
}