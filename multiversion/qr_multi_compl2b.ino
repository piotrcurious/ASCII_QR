#define BYTE_MODE 0x4 // Mode indicator for Byte mode (4 bits)
#define MAX_DATA_SIZE 295 // Max capacity for Version 3-L

// Function to encode data into byte mode binary format
void encodeData(const char* text, uint8_t* data, uint16_t& dataLen, uint8_t version) {
    uint16_t maxCapacity = pgm_read_word(&qrVersions[version].maxMessageSize);

    dataLen = 0;

    // Step 1: Add mode indicator for Byte mode (4 bits)
    data[0] = BYTE_MODE << 4;
    dataLen += 4;

    // Step 2: Add character count indicator (8-16 bits depending on version)
    uint16_t textLen = strlen(text);
    if (version < 10) { // For versions 1-9, 8-bit character count
        data[0] |= textLen; // place character count in low bits of first byte
        dataLen += 8;
    } else { // For versions >=10, 16-bit character count
        data[0] |= (textLen >> 8); // High 8 bits
        data[1] = textLen & 0xFF; // Low 8 bits
        dataLen += 16;
    }

    // Step 3: Convert each character to 8-bit ASCII and append to the data array
    for (uint16_t i = 0; i < textLen && dataLen < maxCapacity * 8; i++) {
        data[dataLen / 8] = text[i];
        dataLen += 8;
    }

    // Step 4: Add terminator and pad the data if necessary
    if (dataLen % 8 != 0) {
        data[dataLen / 8] &= (0xFF << (8 - dataLen % 8)); // Pad final byte if needed
        dataLen += 8 - (dataLen % 8);
    }
    while (dataLen / 8 < maxCapacity) {
        data[dataLen / 8] = 0xEC;
        dataLen += 8;
        if (dataLen / 8 < maxCapacity) {
            data[dataLen / 8] = 0x11;
            dataLen += 8;
        }
    }
}
#define GF_SIZE 256
#define POLY 0x11D  // Polynomial for QR Codes: x^8 + x^4 + x^3 + x^2 + 1

uint8_t gf_exp[2 * GF_SIZE]; // Exponentiation table
uint8_t gf_log[GF_SIZE];     // Logarithm table

// Initialize Galois Field tables
void initGaloisField() {
    uint8_t x = 1;
    for (int i = 0; i < GF_SIZE - 1; i++) {
        gf_exp[i] = x;
        gf_log[x] = i;
        x <<= 1;
        if (x & 0x100) {
            x ^= POLY;
        }
    }
    for (int i = GF_SIZE - 1; i < 2 * GF_SIZE - 1; i++) {
        gf_exp[i] = gf_exp[i - (GF_SIZE - 1)];
    }
}

// Multiply two numbers in GF(256)
uint8_t gfMultiply(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) return 0;
    return gf_exp[gf_log[a] + gf_log[b]];
}

// Divide two numbers in GF(256)
uint8_t gfDivide(uint8_t a, uint8_t b) {
    if (b == 0) return 0; // Division by zero is undefined
    if (a == 0) return 0;
    return gf_exp[(gf_log[a] + GF_SIZE - gf_log[b]) % (GF_SIZE - 1)];
}

#define MAX_ECC_CODEWORDS 30
uint8_t generatorPoly[MAX_ECC_CODEWORDS + 1];

// Generate the Reed-Solomon generator polynomial of specified degree
void generatePolynomial(uint8_t degree) {
    generatorPoly[0] = 1;
    for (uint8_t i = 1; i <= degree; i++) {
        generatorPoly[i] = 0;
    }
    for (uint8_t i = 0; i < degree; i++) {
        for (int j = i; j >= 0; j--) {
            generatorPoly[j + 1] ^= gfMultiply(generatorPoly[j], gf_exp[i]);
        }
    }
}

void generateQRCode(const char* text, uint8_t version) {
    uint8_t data[MAX_MATRIX_SIZE * MAX_MATRIX_SIZE / 8] = {0};
    uint16_t dataLen = 0;
    uint8_t ecc[MAX_ECC_CODEWORDS];

    // Step 1: Encode the data
    encodeData(text, data, dataLen, version);

    // Step 2: Generate the generator polynomial
    uint8_t eccLen = pgm_read_byte(&qrVersions[version].eccSize);
    generatePolynomial(eccLen);

    // Step 3: Calculate error correction codewords
    calculateErrorCorrection(data, dataLen, ecc, eccLen);

    // Step 4: Initialize the QR matrix
    initializeQRMatrix(version);

    // Step 5: Place data and error correction into matrix
    placeDataInMatrix(data, dataLen / 8, pgm_read_byte(&qrVersions[version].size));

    // Step 6: Display matrix (for debugging)
    printQRMatrix(pgm_read_byte(&qrVersions[version].size));
}

void setup() {
    Serial.begin(115200);
    initGaloisField();
    generateQRCode("HELLO", 2);
}

void loop() {
    // Empty loop
}
