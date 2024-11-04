#define QR_SIZE 25           // QR Code Version 2 (25x25 matrix)
#define ECC_SIZE 10          // Reed-Solomon error correction size for Version 2-L
#define MESSAGE_SIZE 13      // Message size (adjust based on your message length)
#define FORMAT_INFO 0b101010000010010  // Example format info with error correction level L and mask pattern 0

uint8_t message[MESSAGE_SIZE] = { 'Q', 'R', ' ', 'C', 'O', 'D', 'E', ' ', 'V', '2', ' ', '!', '!' };  // Example message
uint8_t qrMatrix[QR_SIZE][QR_SIZE];   // QR code matrix
uint8_t ecc[ECC_SIZE];                // Error correction codewords

// Galois Field tables for GF(256)
uint8_t gf_exp[256 * 2];
uint8_t gf_log[256];

// Initialize Galois Field tables for GF(256)
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

// Galois Field multiplication
uint8_t gfMultiply(uint8_t x, uint8_t y) {
    if (x == 0 || y == 0) return 0;
    return gf_exp[gf_log[x] + gf_log[y]];
}

// Reed-Solomon error correction calculation
void computeECC(uint8_t *data, int data_len, uint8_t *ecc, int ecc_len) {
    uint8_t generator[ECC_SIZE + 1] = {1};
    for (int i = 0; i < ecc_len; i++) {
        generator[i + 1] = gf_exp[i];
    }
    for (int i = 0; i < data_len; i++) {
        uint8_t coef = data[i];
        if (coef != 0) {
            for (int j = 0; j < ecc_len; j++) {
                ecc[j] ^= gfMultiply(generator[j + 1], coef);
            }
        }
        for (int j = 0; j < ecc_len - 1; j++) {
            ecc[j] = ecc[j + 1];
        }
        ecc[ecc_len - 1] = 0;
    }
}

// Initialize the QR matrix with finder, alignment, timing patterns
void initializeQRMatrix() {
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            qrMatrix[y][x] = 0;
        }
    }
    addFinderPattern(0, 0);
    addFinderPattern(QR_SIZE - 7, 0);
    addFinderPattern(0, QR_SIZE - 7);
    addAlignmentPattern(QR_SIZE - 10, QR_SIZE - 10);  // Add alignment pattern for Version 2
    addTimingPatterns();
    addFormatInformation(FORMAT_INFO);
}

// Adds a finder pattern at specified coordinates
void addFinderPattern(int x, int y) {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            qrMatrix[y + i][x + j] = (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) ? 1 : 0;
        }
    }
}

// Adds an alignment pattern at specified coordinates
void addAlignmentPattern(int x, int y) {
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            qrMatrix[y + i][x + j] = (abs(i) == 2 || abs(j) == 2 || (i == 0 && j == 0)) ? 1 : 0;
        }
    }
}

// Adds timing patterns along row 6 and column 6
void addTimingPatterns() {
    for (int i = 0; i < QR_SIZE; i++) {
        if (i % 2 == 0) {
            qrMatrix[6][i] = 1;
            qrMatrix[i][6] = 1;
        }
    }
}

// Adds format information around finder patterns
void addFormatInformation(uint16_t format) {
    for (int i = 0; i < 15; i++) {
        int bit = (format >> (14 - i)) & 1;
        if (i < 6) {
            qrMatrix[8][i] = bit;
            qrMatrix[i][8] = bit;
        } else if (i == 6) {
            qrMatrix[8][7] = bit;
            qrMatrix[7][8] = bit;
        } else {
            qrMatrix[8][QR_SIZE - 15 + i] = bit;
            qrMatrix[QR_SIZE - 15 + i][8] = bit;
        }
    }
}

// Inserts data and ECC into QR matrix using a zigzag pattern
void placeDataInQRMatrix(uint8_t *data, int data_len) {
    int dataBitIndex = 0;
    for (int y = QR_SIZE - 1; y > 0; y -= 2) {
        if (y == 6) y--;  // Skip timing pattern
        for (int x = QR_SIZE - 1; x >= 0; x--) {
            if (qrMatrix[y][x] == 0) {
                qrMatrix[y][x] = (data[dataBitIndex / 8] & (1 << (7 - (dataBitIndex % 8)))) ? 1 : 0;
                dataBitIndex++;
                if (dataBitIndex >= data_len * 8) return;
            }
            if (qrMatrix[y - 1][x] == 0) {
                qrMatrix[y - 1][x] = (data[dataBitIndex / 8] & (1 << (7 - (dataBitIndex % 8)))) ? 1 : 0;
                dataBitIndex++;
                if (dataBitIndex >= data_len * 8) return;
            }
        }
    }
}

// Apply mask pattern (e.g., pattern 0) for improved readability
void applyMaskPattern() {
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            if ((x + y) % 2 == 0) qrMatrix[y][x] ^= 1;
        }
    }
}

// Display the QR matrix as ASCII
void printQRMatrix() {
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            Serial.print(qrMatrix[y][x] ? "##" : "  ");
        }
        Serial.println();
    }
}

void setup() {
    Serial.begin(115200);
    initGaloisField();          // Initialize Galois Field
    initializeQRMatrix();       // Initialize QR matrix with finder, alignment, and timing patterns
    
    computeECC(message, MESSAGE_SIZE, ecc, ECC_SIZE);  // Generate ECC
    uint8_t encodedData[MESSAGE_SIZE + ECC_SIZE];
    memcpy(encodedData, message, MESSAGE_SIZE);
    memcpy(encodedData + MESSAGE_SIZE, ecc, ECC_SIZE);

    placeDataInQRMatrix(encodedData, MESSAGE_SIZE + ECC_SIZE);  // Insert data and ECC into matrix
    applyMaskPattern();         // Apply mask pattern
    printQRMatrix();            // Print QR code as ASCII
}

void loop() {
    // Nothing to do in loop
}
