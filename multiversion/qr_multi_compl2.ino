#include <Arduino.h>
#include <pgmspace.h>
#include <string.h>
#include <stdint.h>

#define BYTE_MODE 0x4  // Mode indicator for Byte mode

// Convert input text into binary sequence
void encodeData(const char* text, uint8_t* data, uint16_t& dataLen, uint8_t version) {
    uint8_t textLen = strlen(text);
    uint16_t capacity = pgm_read_word(&qrVersions[version].maxMessageSize);

    // Step 1: Add mode indicator (BYTE_MODE)
    data[0] = BYTE_MODE << 4;
    dataLen = 4;

    // Step 2: Add character count indicator
    data[1] = textLen;
    dataLen += 8;

    // Step 3: Convert each character into 8-bit ASCII
    for (uint8_t i = 0; i < textLen && dataLen < capacity * 8; i++) {
        data[dataLen / 8] = text[i];
        dataLen += 8;
    }

    // Step 4: Pad data if necessary
    while (dataLen % 8 != 0) dataLen++; // Pad with zero bits
    while (dataLen / 8 < capacity) data[dataLen / 8] = 0xEC;
}
