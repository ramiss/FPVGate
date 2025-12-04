#ifndef NODEMODE_H
#define NODEMODE_H

#include <Arduino.h>
#include "laptimer.h"
#include "config.h"

// RotorHazard protocol command constants (must match RHInterface.py exactly)
// READ commands (< 0x50)
#define READ_ADDRESS 0x00
#define READ_FREQUENCY 0x03
#define READ_LAP_STATS 0x05
#define READ_LAP_PASS_STATS 0x0D
#define READ_LAP_EXTREMUMS 0x0E
#define READ_RHFEAT_FLAGS 0x11
#define READ_REVISION_CODE 0x22
#define READ_NODE_RSSI_PEAK 0x23
#define READ_NODE_RSSI_NADIR 0x24
#define READ_ENTER_AT_LEVEL 0x31
#define READ_EXIT_AT_LEVEL 0x32
#define READ_TIME_MILLIS 0x33
#define READ_MULTINODE_COUNT 0x39
#define READ_CURNODE_INDEX 0x3A
#define READ_NODE_SLOTIDX 0x3C
#define READ_FW_VERSION 0x3D
#define READ_FW_BUILDDATE 0x3E
#define READ_FW_BUILDTIME 0x3F
#define READ_FW_PROCTYPE 0x40

// WRITE commands (>= 0x51)
#define WRITE_FREQUENCY 0x51
#define WRITE_ENTER_AT_LEVEL 0x71
#define WRITE_EXIT_AT_LEVEL 0x72
#define SEND_STATUS_MESSAGE 0x75
#define FORCE_END_CROSSING 0x78
#define WRITE_CURNODE_INDEX 0x7A
#define JUMP_TO_BOOTLOADER 0x7E

// Protocol constants
#define NODE_API_LEVEL 35  // RotorHazard API version
#define RHFEAT_FLAGS_VALUE 0x0000  // No special features

// Node settings structure
struct NodeSettings {
    uint16_t vtxFreq = 5800;
    uint8_t enterAtLevel = 120;
    uint8_t exitAtLevel = 100;
};

// Last pass/lap data structure
struct NodeLastPass {
    uint32_t timestamp = 0;
    uint8_t rssiPeak = 0;
    uint16_t lap = 0;
};

class NodeMode {
public:
    NodeMode();
    void begin(LapTimer* timer, Config* config);
    void process();  // Called in main loop
    
private:
    LapTimer* _timer;
    Config* _config;
    NodeSettings _settings;
    NodeLastPass _lastPass;
    uint8_t _nodeIndex = 0;
    uint8_t _slotIndex = 0;
    
    // Serial communication
    void handleSerialInput();
    void handleReadCommand(uint8_t command);
    void handleWriteCommand(uint8_t command, uint8_t* payload, uint8_t len);
    void sendResponse(uint8_t* data, uint8_t len);
    uint8_t getPayloadSize(uint8_t command);
    bool isValidCommand(uint8_t command);
    
    // Message parsing state
    uint8_t _currentCommand = 0;
    uint8_t _payloadBuffer[32];
    uint8_t _payloadIndex = 0;
    uint8_t _expectedPayloadSize = 0;
};

#endif // NODEMODE_H
