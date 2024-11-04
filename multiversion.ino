#include <Arduino.h>

#define MAX_VERSION 30
#define ECC_LEVEL_L 0 // Low error correction level

// Structure to hold QR code version properties
struct QRVersion {
    int size;
    int maxMessageSize;
    int eccSize;
};

// Table with properties for versions 1 to MAX_VERSION
QRVersion qrVersions[MAX_VERSION + 1] = {
    {0, 0, 0},        // Dummy for zero-based index
    {21, 19, 7},      // Version 1
    {25, 34, 10},     // Version 2
    {29, 55, 15},     // Version 3
    // ...Add for all up to Version 30
    {177, 3706, 666}  // Version 30
};

// Alignment pattern positions for versions 1 to 30
const uint8_t alignmentPatternPositions[31][7] = {
    {}, {6, 18}, {6, 22}, {6, 26}, {6, 30}, {6, 34},
    {6, 22, 38}, {6, 24, 42}, {6, 26, 46}, {6, 28, 50},
    // ... Fill in patterns for versions 11 to 30
};

uint8_t qrMatrix[177][177];  // Max size for Version 30
uint8_t ecc[666];            // Max ECC length

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
