#include <Arduino.h>
#include <pgmspace.h>

#define MAX_VERSION 30
#define MAX_MATRIX_SIZE 177  // Max size for Version 30

// QR Version properties stored in PROGMEM
struct QRVersion {
    uint8_t size;            // Matrix size (e.g., 21 for version 1)
    uint16_t maxMessageSize; // Max data size in bytes
    uint8_t eccSize;         // Error correction size in bytes
};

// PROGMEM table for QR versions
const QRVersion qrVersions[MAX_VERSION + 1] PROGMEM = {
    {0, 0, 0},            {21, 19, 7},    {25, 34, 10},    {29, 55, 15},
    {33, 80, 20},         {37, 108, 26},  {41, 136, 36},   {45, 156, 40},
    {49, 194, 48},        {53, 232, 60},  {57, 274, 72},   {61, 324, 80},
    {65, 370, 96},        {69, 428, 104}, {73, 461, 120},  {77, 523, 132},
    {81, 589, 144},       {85, 647, 168}, {89, 721, 180},  {93, 795, 196},
    {97, 861, 224},       {101, 932, 224},{105, 1006, 252},{109, 1094, 270},
    {113, 1174, 300},     {117, 1276, 312},{121, 1370, 336},{125, 1468, 360},
    {129, 1531, 390},     {133, 1631, 420},{177, 3706, 666}
};

// Alignment pattern positions stored in PROGMEM
const uint8_t alignmentPatternPositions[MAX_VERSION + 1][7] PROGMEM = {
    {}, {6, 18}, {6, 22}, {6, 26}, {6, 30}, {6, 34}, {6, 22, 38},
    {6, 24, 42}, {6, 26, 46}, {6, 28, 50}, {6, 30, 54}, {6, 32, 58},
    {6, 34, 62}, {6, 26, 46, 66}, {6, 26, 48, 70}, {6, 26, 50, 74},
    {6, 30, 54, 78}, {6, 30, 56, 82}, {6, 30, 58, 86}, {6, 34, 62, 90},
    {6, 28, 50, 72, 94}, {6, 26, 50, 74, 98}, {6, 30, 54, 78, 102},
    {6, 28, 54, 80, 106}, {6, 32, 58, 84, 110}, {6, 30, 58, 86, 114},
    {6, 34, 62, 90, 118}, {6, 26, 50, 74, 98, 122}, {6, 30, 54, 78, 102, 126},
    {6, 26, 52, 78, 104, 130}
};

// QR matrix
uint8_t qrMatrix[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE];

// Retrieve a specific QR version's properties from PROGMEM
QRVersion getQRVersion(uint8_t version) {
    QRVersion ver;
    memcpy_P(&ver, &qrVersions[version], sizeof(QRVersion));
    return ver;
}

// Get alignment pattern positions from PROGMEM
void getAlignmentPositions(uint8_t version, uint8_t *positions, uint8_t &count) {
    count = sizeof(alignmentPatternPositions[version]);
    memcpy_P(positions, &alignmentPatternPositions[version], count);
}

// Draw a 7x7 finder pattern at (x, y)
void addFinderPattern(int x, int y) {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            qrMatrix[y + i][x + j] = (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) ? 1 : 0;
        }
    }
}

// Place alignment patterns
void addAlignmentPatterns(uint8_t version) {
    uint8_t positions[7], count;
    getAlignmentPositions(version, positions, count);

    for (uint8_t i = 0; i < count; i++) {
        for (uint8_t j = 0; j < count; j++) {
            if ((i == 0 && j == 0) || (i == count - 1 && j == 0) || (i == 0 && j == count - 1)) continue;
            addFinderPattern(positions[i], positions[j]);
        }
    }
}

// Add timing patterns along row and column 6
void addTimingPatterns(uint8_t size) {
    for (int i = 0; i < size; i++) {
        qrMatrix[6][i] = qrMatrix[i][6] = (i % 2 == 0) ? 1 : 0;
    }
}

// Initialize QR code matrix with structural patterns
void initializeQRMatrix(uint8_t version) {
    QRVersion ver = getQRVersion(version);
    memset(qrMatrix, 0, sizeof(qrMatrix));

    addFinderPattern(0, 0);
    addFinderPattern(ver.size - 7, 0);
    addFinderPattern(0, ver.size - 7);
    addAlignmentPatterns(version);
    addTimingPatterns(ver.size);
}

// Print the QR matrix as ASCII art
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
    uint8_t version = 3;  // Choose QR version
    initializeQRMatrix(version);

    QRVersion ver = getQRVersion(version);
    printQRMatrix(ver.size);
}

void loop() {
    // No operation needed in loop
}
