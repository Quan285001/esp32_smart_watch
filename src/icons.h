#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>

// Vẽ icon Mặt Trời phóng to gấp đôi
inline void drawMiniSun(TFT_eSPI &tft, int x, int y, uint16_t color) {
    tft.fillCircle(x, y, 6, color); // Tăng bán kính tròn trung tâm lên 6 pixel
    // Vẽ các tia nắng dài và rõ hơn
    tft.drawFastVLine(x, y - 10, 4, color);
    tft.drawFastVLine(x, y + 7, 4, color);
    tft.drawFastHLine(x - 10, y, 4, color);
    tft.drawFastHLine(x + 7, y, 4, color);
    // Thêm tia chéo góc cho sinh động
    tft.drawPixel(x - 6, y - 6, color);
    tft.drawPixel(x + 6, y - 6, color);
    tft.drawPixel(x - 6, y + 6, color);
    tft.drawPixel(x + 6, y + 6, color);
}

// Vẽ icon Mây phóng to gấp đôi
inline void drawMiniCloud(TFT_eSPI &tft, int x, int y, uint16_t color) {
    tft.fillCircle(x - 6, y + 2, 5, color);
    tft.fillCircle(x + 5, y, 7, color);
    tft.fillCircle(x, y - 4, 6, color);
    tft.fillRect(x - 8, y, 16, 8, color);
}

// Vẽ icon Mưa phóng to gấp đôi
inline void drawMiniRain(TFT_eSPI &tft, int x, int y, uint16_t color) {
    drawMiniCloud(tft, x, y - 3, color);
    // Các hạt mưa to hơn, dài hơn
    tft.drawFastVLine(x - 6, y + 4, 5, 0x001F);
    tft.drawFastVLine(x, y + 4, 5, 0x001F);
    tft.drawFastVLine(x + 6, y + 4, 5, 0x001F);
}

// Vẽ icon Giông bão phóng to gấp đôi
inline void drawMiniThunder(TFT_eSPI &tft, int x, int y, uint16_t color) {
    drawMiniCloud(tft, x, y - 4, color);
    // Tia chớp lớn dễ nhìn
    tft.drawLine(x, y + 2, x - 4, y + 8, 0xFEE0);
    tft.drawLine(x - 4, y + 8, x + 2, y + 8, 0xFEE0);
    tft.drawLine(x + 2, y + 8, x - 2, y + 14, 0xFEE0);
}

// Vẽ icon Nắng xen lẫn Mây (Phóng to gấp đôi, cực kỳ sinh động)
inline void drawMiniSunCloud(TFT_eSPI &tft, int x, int y, uint16_t color) {
    // Vẽ mặt trời nhỏ ở góc trên bên trái
    tft.fillCircle(x - 4, y - 4, 5, 0xFDE0); // Màu vàng cam
    tft.drawPixel(x - 4, y - 11, 0xFDE0);
    tft.drawPixel(x - 11, y - 4, 0xFDE0);
    tft.drawPixel(x - 9, y - 9, 0xFDE0);

    // Vẽ đám mây chồng lên phía trước mặt trời
    tft.fillCircle(x - 3, y + 2, 5, color);
    tft.fillCircle(x + 5, y, 7, color);
    tft.fillCircle(x + 1, y - 3, 6, color);
    tft.fillRect(x - 6, y, 13, 8, color);
}

// Vẽ icon Trăng tròn ban đêm (Code 0 - Ban đêm)
inline void drawMiniMoon(TFT_eSPI &tft, int x, int y, uint16_t color) {
    tft.fillCircle(x, y, 6, color);
    // Tạo hiệu ứng khuyết nhẹ hoặc ánh trăng lưỡi liềm nghệ thuật bên trong nếu muốn, hoặc trăng tròn sáng rực
    tft.fillCircle(x + 2, y - 1, 5, 0x0000); // Khoét một chút để tạo hình trăng khuyết/trăng lưỡi liềm sang trọng
    tft.fillCircle(x, y, 5, color);
}

// Vẽ icon Trăng khuyết
inline void drawMiniCrescentMoon(TFT_eSPI &tft, int x, int y, uint16_t color) {
    tft.fillCircle(x, y, 6, color);
    tft.fillCircle(x + 3, y - 1, 5, 0x0000); // Khoét tạo hình lưỡi liềm sắc sảo
}

// Vẽ icon Trăng kèm Mây về đêm (Code 1, 2, 3 - Ban đêm)
inline void drawMiniMoonCloud(TFT_eSPI &tft, int x, int y, uint16_t color) {
    // Vẽ trăng khuyết ở góc trên
    tft.fillCircle(x - 4, y - 4, 5, 0xFCE0); // Màu vàng trăng sáng
    tft.fillCircle(x - 1, y - 5, 4, 0x0000); // Khoét lưỡi liềm
    
    // Vẽ đám mây che phía trước
    tft.fillCircle(x - 3, y + 2, 5, color);
    tft.fillCircle(x + 5, y, 7, color);
    tft.fillCircle(x + 1, y - 3, 6, color);
    tft.fillRect(x - 6, y, 13, 8, color);
}