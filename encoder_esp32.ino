#define QR_SIZE 21           // QR Code Version 1 (21x21 matrix)
#define ECC_SIZE 7           // Reed-Solomon error correction code size (example for Version 1-L)
#define MESSAGE_SIZE 8       // Message size (customize based on your message length)

uint8_t message[MESSAGE_SIZE] = { 'H', 'E', 'L', 'L', 'O', '1', '2', '3' };  // Example message
uint8_t qrMatrix[QR_SIZE][QR_SIZE];  // QR code matrix
uint8_t ecc[ECC_SIZE];               // Error correction codewords

// Galois Field tables for GF(256)
uint8_t gf_exp[256 * 2];
uint8_t gf_log[256];

// Galois Field initialization for GF(256)
void initGaloisField() {
    uint8_t x = 1;
    for (int i = 0; i < 255; i++) {
        gf_exp[i] = x;
        gf_log[x] = i;
        x <<= 1;
        if (x & 0x100) x ^= 0x11d;  // Primitive polynomial x^8 + x^4 + x^3 + x^2 + 1
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

// Calculate Reed-Solomon error correction codewords
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

// Initialize the QR matrix with essential patterns
void initializeQRMatrix() {
    // Clear matrix
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            qrMatrix[y][x] = 0;
        }
    }
    // Finder patterns in three corners
    addFinderPattern(0, 0);
    addFinderPattern(QR_SIZE - 7, 0);
    addFinderPattern(0, QR_SIZE - 7);
    // Timing patterns
    addTimingPatterns();
}

// Add a 7x7 finder pattern
void addFinderPattern(int x, int y) {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            qrMatrix[y + i][x + j] = (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) ? 1 : 0;
        }
    }
}

// Add timing patterns along row 6 and column 6
void addTimingPatterns() {
    for (int i = 0; i < QR_SIZE; i++) {
        if (i % 2 == 0) {
            qrMatrix[6][i] = 1;    // Horizontal timing pattern
            qrMatrix[i][6] = 1;    // Vertical timing pattern
        }
    }
}

// Add encoded data and ECC into QR matrix
void placeDataInQRMatrix(uint8_t *data, int data_len) {
    int dataBitIndex = 0;
    for (int y = QR_SIZE - 1; y >= 0; y -= 2) {
        if (y == 6) y--;  // Skip the timing pattern row
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

// Apply mask pattern (0) for improved readability
void applyMaskPattern() {
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            if ((x + y) % 2 == 0) qrMatrix[y][x] ^= 1;  // Example mask
        }
    }
}

// Display the QR code matrix as ASCII art
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
    initGaloisField();         // Initialize Galois Field for ECC
    initializeQRMatrix();      // Initialize QR matrix with finder and timing patterns

    computeECC(message, MESSAGE_SIZE, ecc, ECC_SIZE); // Generate ECC
    
    uint8_t encodedData[MESSAGE_SIZE + ECC_SIZE];
    memcpy(encodedData, message, MESSAGE_SIZE);       // Copy message to encoded data
    memcpy(encodedData + MESSAGE_SIZE, ecc, ECC_SIZE); // Append ECC

    placeDataInQRMatrix(encodedData, MESSAGE_SIZE + ECC_SIZE); // Insert data and ECC into QR matrix
    applyMaskPattern();        // Apply a mask pattern

    printQRMatrix();           // Print QR code in ASCII
}

void loop() {
    // Nothing to do in loop
}
