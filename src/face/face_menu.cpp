#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

struct MenuItem {
    const char* label;
    bool status; 
};

void drawMenuScreen(int selectedIndex, bool wifiStatus) {
    tft.fillScreen(0x0000); // Nền đen

    int centerX = 120;
    
    // Tiêu đề Menu
    tft.setTextColor(0xFFFF, 0x0000);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("MENU", centerX, 25, 2);
    tft.drawFastHLine(60, 40, 120, 0x3186);

    // Danh sách 4 mục menu gọn gàng (Đã bỏ Weather Info, thêm System Info)
    MenuItem items[4] = {
        {"Wi-Fi Mode", wifiStatus},
        {"Watch Face", false},
        {"Sleep Mode", false},
        {"System Info", false}
    };

    int startY = 75;
    int itemHeight = 36;

    for (int i = 0; i < 4; i++) {
        int yPos = startY + (i * itemHeight);

        if (i == selectedIndex) {
            tft.fillRoundRect(30, yPos - 12, 180, 28, 6, 0x2104);
            tft.drawRoundRect(30, yPos - 12, 180, 28, 6, 0xFDE0); 
            tft.setTextColor(0xFDE0, 0x2104);
        } else {
            tft.setTextColor(0xCE79, 0x0000);
        }

        tft.setTextDatum(ML_DATUM);
        tft.drawString(items[i].label, 42, yPos, 2);

        if (i == 0) {
            tft.setTextDatum(MR_DATUM);
            tft.setTextColor(wifiStatus ? 0x07E0 : 0xF800, i == selectedIndex ? 0x2104 : 0x0000);
            tft.drawString(wifiStatus ? "ON" : "OFF", 195, yPos, 2);
        }
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0x7BEF, 0x0000);
    tft.drawString("[Press: Next | Hold: Select]", centerX, 205, 1);
}