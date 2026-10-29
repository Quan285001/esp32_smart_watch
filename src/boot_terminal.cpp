#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

void showBootTerminal() {
    tft.fillScreen(0x0000); // Xóa nền đen tuyệt đối
    
    // Tiêu đề Terminal
    tft.setTextColor(0x07E0, 0x0000); // Xanh lá Matrix
    tft.setTextDatum(MC_DATUM);
    tft.drawString("=== START UP ===", 120, 18, 1);
    tft.drawFastHLine(25, 28, 190, 0x3186);

    int startY = 42;
    int lineHeight = 15;
    int currentY = startY;

    // Hàm phụ trợ in từng dòng log có hiệu ứng delay nhẹ
    auto logLine = [&](const char* msg, uint16_t color) {
        tft.setTextDatum(ML_DATUM);
        tft.setTextColor(color, 0x0000);
        tft.drawString(msg, 22, currentY, 1);
        currentY += lineHeight;
        delay(80); 
    };

    logLine("[INIT] Booting Kernel...", 0xFFFF);
    logLine("[OK] GC9A01 Display init", 0x07E0);

    // Lấy dung lượng RAM trống thực tế ngay lúc boot và ghép vào chuỗi
    uint32_t freeHeapVal = ESP.getFreeHeap();
    char heapMsg[32];
    sprintf(heapMsg, "[OK] SRAM Heap: %d KB", freeHeapVal / 1024);
    logLine(heapMsg, 0x07E0);

    logLine("[BLE] Stack initialized", 0xFDE0);
    logLine("[WIFI] Connecting AP...", 0xFDE0);
    logLine("[NTP] Syncing UTC+7...", 0xFDE0);
    logLine("[SYS] Running...", 0x07E0);
    
    delay(400); 
}