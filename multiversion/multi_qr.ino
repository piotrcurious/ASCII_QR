#include <Arduino.h>
#include <pgmspace.h>

#define MAX_VERSION 30

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

// QR Matrix
uint8_t qrMatrix[177][177];  // Max size for Version 30

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

// Add a finder pattern at a specific location
void addFinderPattern(int x, int y) {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            qrMatrix[y + i][x + j] = (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) ? 1 : 0;
        }
    }
}

// Add alignment patterns based on the version
void addAlignmentPatterns(uint8_t version) {
    uint8_t positions[7];
    getAlignmentPatternPositions(version, positions);
    int posCount = sizeof(alignmentPatternPositions[version]) / sizeof(uint8_t);

    for (int i = 0; i < posCount; i++) {
        for (int j = 0; j < posCount; j++) {
            if ((i == 0 && j == 0) || (i == posCount - 1 && j == 0) || (i == 0 && j == posCount - 1)) continue;
            int px = positions[i], py = positions[j];
            addFinderPattern(px, py);
        }
    }
}

// Adds timing patterns along row and column 6
void addTimingPatterns(uint8_t matrixSize) {
    for (int i = 0; i < matrixSize; i++) {
        qrMatrix[6][i] = (i % 2 == 0) ? 1 : 0;
        qrMatrix[i][6] = (i % 2 == 0) ? 1 : 0;
    }
}

// Initialize and print QR code matrix
void setupQR(uint8_t version) {
    QRVersion qrVer = getQRVersion(version);
    memset(qrMatrix, 0, sizeof(qrMatrix));

    addFinderPattern(0, 0);
    addFinderPattern(qrVer.size - 7, 0);
    addFinderPattern(0, qrVer.size - 7);
    addAlignmentPatterns(version);
    addTimingPatterns(qrVer.size);
}

// Print QR matrix in ASCII
void printQRMatrix(uint8_t size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            Serial.print(qrMatrix[y][x] ? "##" : "  ");
        }
        Serial.println();
    }
}

void setup() {
    Serial.begin(115200);

    uint8_t version = 3;  // Example version
    setupQR(version);
    QRVersion qrVer = getQRVersion(version);
    printQRMatrix(qrVer.size);
}

void loop() {
    // No loop action needed
}
