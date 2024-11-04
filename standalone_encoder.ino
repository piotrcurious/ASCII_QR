#define QR_SIZE 21       // QR Code Version 1 (21x21 matrix)
#define ECC_SIZE 7       // Reed-Solomon error correction size (example)

uint8_t message[] = { 'H', 'E', 'L', 'L', 'O' };  // Simple example message
uint8_t qrMatrix[QR_SIZE][QR_SIZE];               // QR matrix
uint8_t ecc[ECC_SIZE];                            // Error correction codewords

// Galois Field tables
uint8_t gf_exp[256 * 2];
uint8_t gf_log[256];

// Initializes Galois Field tables for GF(256)
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

// Initialize the QR matrix with basic patterns (finder, alignment)
void initializeQRMatrix() {
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            qrMatrix[y][x] = 0;
        }
    }
    // Add finder patterns in corners
    addFinderPattern(0, 0);
    addFinderPattern(QR_SIZE - 7, 0);
    addFinderPattern(0, QR_SIZE - 7);
}

// Add a 7x7 finder pattern
void addFinderPattern(int x, int y) {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            qrMatrix[y + i][x + j] = (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) ? 1 : 0;
        }
    }
}

// Place encoded data and ECC into QR matrix
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

// Display the QR code matrix in ASCII
void printQRMatrix() {
    for (int y = 0; y < QR_SIZE; y++) {
        for (int x = 0; x < QR_SIZE; x++) {
            Serial.print(qrMatrix[y][x] ? "##" : "  ");
        }
        Serial.println();
    }
}

void setup() {
    Serial.begin(9600);
    initGaloisField();   // Initialize Galois Field
    initializeQRMatrix(); // Initialize QR matrix with finder patterns
    
    computeECC(message, sizeof(message), ecc, ECC_SIZE); // Generate ECC
    
    uint8_t encodedData[MESSAGE_SIZE + ECC_SIZE];
    memcpy(encodedData, message, sizeof(message));      // Copy message to encoded data
    memcpy(encodedData + sizeof(message), ecc, ECC_SIZE); // Append ECC

    placeDataInQRMatrix(encodedData, MESSAGE_SIZE + ECC_SIZE); // Place data in QR matrix
    printQRMatrix();   // Print QR code as ASCII art
}

void loop() {
    // Nothing to do in loop
}
