#ifndef BOARD_DISPLAYS_H
#define BOARD_DISPLAYS_H

#include <Arduino.h>
#include "config/config.h"

// Board-specific display initialization
// Currently supports NuclearCounter OLED display
class BoardDisplays {
public:
    BoardDisplays();

#if defined(BOARD_NUCLEARCOUNTER)
    // Initialize NuclearCounter 1.3" OLED display (SH1106 128x64)
    // Shows device name, IP address, SSID, and status
    void initNuclearCounter(const String& ssid);
#endif

private:
    // No state needed - static display initialization only
};

#endif // BOARD_DISPLAYS_H
