#include "board_displays.h"
#include <WiFi.h>

#if defined(BOARD_NUCLEARCOUNTER)
#include <Wire.h>
#include <U8g2lib.h>
#endif

BoardDisplays::BoardDisplays() {
    // Constructor
}

#if defined(BOARD_NUCLEARCOUNTER)
void BoardDisplays::initNuclearCounter(const String& ssid) {
    // 1.3" I2C OLED, SH1106 128x64.
    // Use hardware I2C via U8g2, explicitly binding SCL=9, SDA=8.
    static U8G2_SH1106_128X64_NONAME_F_HW_I2C ncDisplay(
        U8G2_R0,
        /* reset=*/ U8X8_PIN_NONE,
        /* clock=*/ 9,
        /* data=*/ 8
    );

    ncDisplay.begin();
    ncDisplay.clearBuffer();

    // Use a small, readable font
    ncDisplay.setFont(u8g2_font_6x10_tf);

    // Line 1: Device name
    ncDisplay.drawStr(0, 10, "NuclearCounter SFOS");

    // Line 2: IP address (AP IP, usually 192.168.4.1)
    char ipLine[24];
    snprintf(ipLine, sizeof(ipLine), "IP: %s", WiFi.softAPIP().toString().c_str());
    ncDisplay.drawStr(0, 22, ipLine);

    // Line 3: SSID
    char ssidLine[24];
    snprintf(ssidLine, sizeof(ssidLine), "SSID: %s", ssid.c_str());
    ncDisplay.drawStr(0, 34, ssidLine);

    // Line 4: Status line (placeholder for now)
    ncDisplay.drawStr(0, 46, "Status: READY");

    ncDisplay.sendBuffer();
}
#endif
