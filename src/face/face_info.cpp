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
    tft.drawString("SYSTEM INFO", centerX, 20, 2);
    tft.drawFastHLine(40, 32, 160, 0x3186);

    // Lấy thông số hệ thống
    unsigned long uptimeSeconds = millis() / 1000;
    int hours = uptimeSeconds / 3600;
    int minutes = (uptimeSeconds % 3600) / 60;
    int seconds = uptimeSeconds % 60;
    
    char uptimeStr[16];
    sprintf(uptimeStr, "%02d:%02d:%02d", hours, minutes, seconds);

    uint32_t freeHeap = ESP.getFreeHeap();
    
    // Lấy tên Wi-Fi (SSID) và IP nếu đã kết nối
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    String wifiName = isConnected ? WiFi.SSID() : "Disconnected";
    String ipStr = isConnected ? WiFi.localIP().toString() : "No IP";

    // In thông số ra màn hình (Căn chỉnh tọa độ tối ưu cho màn tròn 240x240)
    tft.setTextDatum(ML_DATUM);
    int startY = 48;
    int spacing = 24;

    // 1. Uptime
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString("Uptime:", 25, startY, 1);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.drawString(uptimeStr, 90, startY, 2);

    // 2. Free RAM (Heap)
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString("Free RAM:", 25, startY + spacing, 1);
    tft.setTextColor(0xFDE0, 0x0000);
    char heapStr[20];
    sprintf(heapStr, "%d KB", freeHeap / 1024);
    tft.drawString(heapStr, 90, startY + spacing, 2);

    // 3. Wi-Fi SSID (Tên mạng đang kết nối)
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString("Wi-Fi:", 25, startY + (spacing * 2), 1);
    tft.setTextColor(isConnected ? 0x07E0 : 0xF800, 0x0000); // Xanh lá nếu có mạng, Đỏ nếu mất kết nối
    tft.drawString(wifiName.c_str(), 90, startY + (spacing * 2), 1);

    // 4. IP Address
    tft.setTextColor(0xCE79, 0x0000);
    tft.drawString("IP:", 25, startY + (spacing * 3), 1);
    tft.setTextColor(0x07E0, 0x0000);
    tft.drawString(ipStr.c_str(), 90, startY + (spacing * 3), 1);

    // Hướng dẫn quay lại
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0x7BEF, 0x0000);
    tft.drawString("[Press: Back to Menu]", centerX, 212, 1);
}