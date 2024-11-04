#define QR_SIZE 25           // QR Code Version 2 (25x25 matrix)
#define ECC_SIZE 10          // Reed-Solomon error correction size for Version 2-L
#define MAX_MESSAGE_SIZE 13  // Maximum message size for Version 2-L with alphanumeric encoding

uint8_t message[MAX_MESSAGE_SIZE] = { 'H', 'E', 'L', 'L', 'O', ' ', 'Q', 'R', ' ', 'C', 'O', 'D', 'E' };
uint8_t qrMatrix[QR_SIZE][QR_SIZE];  // QR code matrix
uint8_t ecc[ECC_SIZE];               // Error correction codewords

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

// Alphanumeric encoding to binary for QR Code
void encodeAlphanumeric(uint8_t *message, int length, uint8_t *encodedData, int *dataLength) {
    int bitIndex = 0;
    for (int i = 0; i < length; i += 2) {
        int value = (gf_log[message[i]] * 45) + (i + 1 < length ? gf_log[message[i + 1]] : 0);
        for (int j = 10; j >= 0; j--) {
            if (bitIndex < QR_SIZE * QR_SIZE) {
                encodedData[bitIndex / 8] |= (value & (1 << j)) ? (1 << (7 - (bitIndex % 8))) : 0;
                bitIndex++;
            }
        }
    }
    *dataLength = bitIndex / 8 + 1;
}

// Calculate Reed-Solomon ECC
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

// Initialize QR matrix with finder, alignment, and timing patterns
void initializeQRMatrix() {
    memset(qrMatrix, 0, sizeof(qrMatrix));
    addFinderPattern(0, 0);
    addFinderPattern(QR_SIZE - 7, 0);
    addFinderPattern(0, QR_SIZE - 7);
    addAlignmentPattern(QR_SIZE - 10, QR_SIZE - 10);
    addTimingPatterns();
    addFormatInformation(0b101010000010010);  // Example fixed format info
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
        qrMatrix[6][i] = qrMatrix[i][6] = (i % 2 == 0) ? 1 : 0;
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

// Select the best mask by applying each and scoring for minimal visual artifacts
int selectBestMask(uint8_t *matrix) {
    int bestMask = 0, bestScore = INT_MAX;
    for (int i = 0; i < 8; i++) {
        int score = scoreMask(i, matrix);
        if (score < bestScore) {
            bestScore = score;
            bestMask = i;
        }
    }
    return bestMask;
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
    initGaloisField();
    initializeQRMatrix();

    int dataLength;
    uint8_t encodedData[MAX_MESSAGE_SIZE + ECC_SIZE];
    encodeAlphanumeric(message, MAX_MESSAGE_SIZE, encodedData, &dataLength);

    computeECC(encodedData, dataLength, ecc, ECC_SIZE);
    memcpy(encodedData + dataLength, ecc, ECC_SIZE);

    placeDataInQRMatrix(encodedData, dataLength + ECC_SIZE);
    printQRMatrix();
}

void loop() {
    // No loop action needed
}