#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// Hàm lưu hoặc cập nhật mạng Wi-Fi vào bộ nhớ Flash vĩnh viễn
inline void saveNetworkToFlash(String ssid, String pass) {
    Preferences prefs;
    prefs.begin("wifi_store", false);
    int count = prefs.getInt("count", 0);
    bool found = false;

    for (int i = 0; i < count; i++) {
        if (prefs.getString(("s_" + String(i)).c_str(), "") == ssid) {
            prefs.putString(("p_" + String(i)).c_str(), pass);
            found = true;
            break;
        }
    }

    if (!found) {
        prefs.putString(("s_" + String(count)).c_str(), ssid);
        prefs.putString(("p_" + String(count)).c_str(), pass);
        prefs.putInt("count", count + 1);
    }
    prefs.end();
}

// Hàm tự động quét không gian, so khớp với danh sách đã lưu để tự kết nối
inline bool tryConnectKnownWiFi(String &connectedSSID) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks();
    if (n == 0) {
        WiFi.mode(WIFI_OFF);
        return false;
    }

    Preferences prefs;
    prefs.begin("wifi_store", true);
    int count = prefs.getInt("count", 0);

    for (int i = 0; i < n; i++) {
        String scannedSSID = WiFi.SSID(i);
        
        for (int j = 0; j < count; j++) {
            String savedSSID = prefs.getString(("s_" + String(j)).c_str(), "");
            String savedPass = prefs.getString(("p_" + String(j)).c_str(), "");

            if (scannedSSID == savedSSID) {
                WiFi.begin(savedSSID.c_str(), savedPass.c_str());
                
                unsigned long startAttemptTime = millis();
                while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 6000) {
                    delay(500);
                }

                if (WiFi.status() == WL_CONNECTED) {
                    connectedSSID = savedSSID;
                    prefs.end();
                    WiFi.scanDelete();
                    return true; // Kết nối thành công!
                }
                WiFi.disconnect();
            }
        }
    }
    
    prefs.end();
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF); // Không thấy mạng quen -> Tắt Wi-Fi ngay để tiết kiệm pin
    return false;
}

#endif