#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

struct MenuItem {
    const char* label;
    bool hasStatus;
    bool status; 
};

void drawMenuScreen(int selectedIndex, bool wifiStatus, bool btStatus) {
    tft.fillScreen(0x0000); // <--- CỰC KỲ QUAN TRỌNG: Xóa sạch màn hình trước khi vẽ menu để chống nhòe đè chữ!

    int centerX = 120;
    
    // Tiêu đề Menu
    tft.setTextColor(0xFFFF, 0x0000);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("MENU", centerX, 20, 2);
    tft.drawFastHLine(60, 34, 120, 0x3186);

    // 5 mục menu
    MenuItem items[5] = {
        {"Back to Face", false, false},
        {"Wi-Fi Mode",   true,  wifiStatus},
        {"Bluetooth",    true,  btStatus},
        {"Sleep Mode",   false, false},
        {"System Info",  false, false}
    };

    int startY = 52;
    int itemHeight = 28;

    for (int i = 0; i < 5; i++) {
        int yPos = startY + (i * itemHeight);

        if (i == selectedIndex) {
            tft.fillRoundRect(22, yPos - 10, 196, 24, 5, 0x2104);
            tft.drawRoundRect(22, yPos - 10, 196, 24, 5, 0xFDE0); 
            tft.setTextColor(0xFDE0, 0x2104);
        } else {
            tft.fillRoundRect(22, yPos - 10, 196, 24, 5, 0x0000);
            tft.setTextColor(0xCE79, 0x0000);
        }

        tft.setTextDatum(ML_DATUM);
        tft.drawString(items[i].label, 32, yPos, 2);

        // Hiển thị trạng thái ON/OFF
        if (items[i].hasStatus) {
            tft.setTextDatum(MR_DATUM);
            uint16_t statusColor = items[i].status ? 0x07E0 : 0xF800;
            uint16_t bgColor = (i == selectedIndex) ? 0x2104 : 0x0000;
            tft.setTextColor(statusColor, bgColor);
            tft.drawString(items[i].status ? "ON" : "OFF", 205, yPos, 2);
        }
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0x7BEF, 0x0000);
    tft.drawString("[Press: Next | Hold: Select]", centerX, 202, 1);
}