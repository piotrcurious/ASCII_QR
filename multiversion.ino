#include <Arduino.h>
include <pgmspace.h>

#define MAX_VERSION 30
#define ECC_LEVEL_L 0 // Low error correction level

// Structure to hold QR code version properties in PROGMEM
struct QRVersion {
    uint8_t size;            // Matrix size (21 for version 1, etc.)
    uint16_t maxMessageSize; // Maximum data size in bytes
    uint8_t eccSize;         // Error correction size in bytes
};

// Table with properties for versions 1 to MAX_VERSION in PROGMEM
const QRVersion qrVersions[MAX_VERSION + 1] PROGMEM = {
    {0, 0, 0},            // Dummy entry for zero-based index
    {21, 19, 7},          // Version 1
    {25, 34, 10},         // Version 2
    {29, 55, 15},         // Version 3
    {33, 80, 20},         // Version 4
    {37, 108, 26},        // Version 5
    {41, 136, 36},        // Version 6
    {45, 156, 40},        // Version 7
    {49, 194, 48},        // Version 8
    {53, 232, 60},        // Version 9
    {57, 274, 72},        // Version 10
    {61, 324, 80},        // Version 11
    {65, 370, 96},        // Version 12
    {69, 428, 104},       // Version 13
    {73, 461, 120},       // Version 14
    {77, 523, 132},       // Version 15
    {81, 589, 144},       // Version 16
    {85, 647, 168},       // Version 17
    {89, 721, 180},       // Version 18
    {93, 795, 196},       // Version 19
    {97, 861, 224},       // Version 20
    {101, 932, 224},      // Version 21
    {105, 1006, 252},     // Version 22
    {109, 1094, 270},     // Version 23
    {113, 1174, 300},     // Version 24
    {117, 1276, 312},     // Version 25
    {121, 1370, 336},     // Version 26
    {125, 1468, 360},     // Version 27
    {129, 1531, 390},     // Version 28
    {133, 1631, 420},     // Version 29
    {177, 3706, 666}      // Version 30
};

// Alignment pattern positions for versions 1 to 30 in PROGMEM
const uint8_t alignmentPatternPositions[MAX_VERSION + 1][7] PROGMEM = {
    {},                 // No alignment for version 1
    {6, 18},            // Version 2
    {6, 22},            // Version 3
    {6, 26},            // Version 4
    {6, 30},            // Version 5
    {6, 34},            // Version 6
    {6, 22, 38},        // Version 7
    {6, 24, 42},        // Version 8
    {6, 26, 46},        // Version 9
    {6, 28, 50},        // Version 10
    {6, 30, 54},        // Version 11
    {6, 32, 58},        // Version 12
    {6, 34, 62},        // Version 13
    {6, 26, 46, 66},    // Version 14
    {6, 26, 48, 70},    // Version 15
    {6, 26, 50, 74},    // Version 16
    {6, 30, 54, 78},    // Version 17
    {6, 30, 56, 82},    // Version 18
    {6, 30, 58, 86},    // Version 19
    {6, 34, 62, 90},    // Version 20
    {6, 28, 50, 72, 94},  // Version 21
    {6, 26, 50, 74, 98},  // Version 22
    {6, 30, 54, 78, 102}, // Version 23
    {6, 28, 54, 80, 106}, // Version 24
    {6, 32, 58, 84, 110}, // Version 25
    {6, 30, 58, 86, 114}, // Version 26
    {6, 34, 62, 90, 118}, // Version 27
    {6, 26, 50, 74, 98, 122}, // Version 28
    {6, 30, 54, 78, 102, 126}, // Version 29
    {6, 26, 52, 78, 104, 130}  // Version 30
};

uint8_t qrMatrix[177][177];  // Max size for Version 30
uint8_t ecc[666];            // Max ECC length

// Function to retrieve version properties from PROGMEM
QRVersion getQRVersion(uint8_t version) {
    QRVersion v;
    memcpy_P(&v, &qrVersions[version], sizeof(QRVersion));
    return v;
}

// Function to retrieve alignment pattern positions from PROGMEM
void getAlignmentPatternPositions(uint8_t version, uint8_t* positions) {
    memcpy_P(positions, &alignmentPatternPositions[version], sizeof(alignmentPatternPositions[version]));
}

// Initialize Galois Field tables for GF(256)
uint8_t gf_exp[256 * 2];
uint8_t gf_log[256];

void initGaloisField() {
    uint8_t x = 1;
    for (int i = 0; i < 255; i++) {
        gf_exp[i] = x;
        gf_log[x] = i;
        x <<= 1;
        if (x & 0x100) x ^= 0x11d;
    }
    for (int i = 255; i < 512; i++) {
        gf_exp[i] = gf_exp[i - 255];
    }
}

// Multiply two elements in GF(256)
uint8_t gfMultiply(uint8_t x, uint8_t y) {
    if (x == 0 || y == 0) return 0;
    return gf_exp[gf_log[x] + gf_log[y]];
}

// Set the QR code version
int qrVersion = 1;   // Default version

void setQRVersion(int version) {
    qrVersion = constrain(version, 1, MAX_VERSION);
    memset(qrMatrix, 0, sizeof(qrMatrix));
}

// Add a finder pattern at a specific location
void addFinderPattern(int x, int y) {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            qrMatrix[y + i][x + j] = (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) ? 1 : 0;
        }
    }
}

// Add alignment patterns based on the version
void addAlignmentPatterns() {
    const uint8_t *positions = alignmentPatternPositions[qrVersion];
    int posCount = sizeof(alignmentPatternPositions[qrVersion]) / sizeof(uint8_t);

    for (int i = 0; i < posCount; i++) {
        for (int j = 0; j < posCount; j++) {
            if ((i == 0 && j == 0) || (i == posCount - 1 && j == 0) || (i == 0 && j == posCount - 1)) continue;
            int px = positions[i], py = positions[j];
            addFinderPattern(px, py);
        }
    }
}

// Adds timing patterns along row and column 6
void addTimingPatterns() {
    for (int i = 0; i < qrVersions[qrVersion].size; i++) {
        qrMatrix[6][i] = (i % 2 == 0) ? 1 : 0;
        qrMatrix[i][6] = (i % 2 == 0) ? 1 : 0;
    }
}

// Encode data into binary format and place it in the matrix
void encodeData(uint8_t *data, int length) {
    int maxDataLength = qrVersions[qrVersion].maxMessageSize;
    length = constrain(length, 0, maxDataLength);
    int dataBitIndex = 0;

    for (int y = qrVersions[qrVersion].size - 1; y > 0; y -= 2) {
        if (y == 6) y--; // Skip timing
        for (int x = qrVersions[qrVersion].size - 1; x >= 0; x--) {
            if (qrMatrix[y][x] == 0 && dataBitIndex < length * 8) {
                qrMatrix[y][x] = (data[dataBitIndex / 8] & (1 << (7 - (dataBitIndex % 8)))) ? 1 : 0;
                dataBitIndex++;
            }
        }
    }
}

// Display QR matrix in ASCII
void printQRMatrix() {
    for (int y = 0; y < qrVersions[qrVersion].size; y++) {
        for (int x = 0; x < qrVersions[qrVersion].size; x++) {
            Serial.print(qrMatrix[y][x] ? "##" : "  ");
        }
        Serial.println();
    }
}

void setup() {
    Serial.begin(115200);
    initGaloisField();

    setQRVersion(3);  // Example version (change as needed)
    
    // Initialize patterns
    addFinderPattern(0, 0);
    addFinderPattern(qrVersions[qrVersion].size - 7, 0);
    addFinderPattern(0, qrVersions[qrVersion].size - 7);
    addAlignmentPatterns();
    addTimingPatterns();

    uint8_t message[20] = { 'H', 'e', 'l', 'l', 'o', ' ', 'Q', 'R', '!' }; // Example message
    encodeData(message, sizeof(message));
    
    printQRMatrix();
}

void loop() {
    // No loop action needed
}
