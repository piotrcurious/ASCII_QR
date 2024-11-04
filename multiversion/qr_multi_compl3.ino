// Apply mask pattern to the QR matrix
void applyMaskPattern(uint8_t pattern, uint8_t size) {
    for (uint8_t row = 0; row < size; row++) {
        for (uint8_t col = 0; col < size; col++) {
            bool invert = false;
            switch (pattern) {
                case 0: invert = (row + col) % 2 == 0; break;
                case 1: invert = row % 2 == 0; break;
                case 2: invert = col % 3 == 0; break;
                case 3: invert = (row + col) % 3 == 0; break;
                case 4: invert = (row / 2 + col / 3) % 2 == 0; break;
                case 5: invert = (row * col) % 2 + (row * col) % 3 == 0; break;
                case 6: invert = ((row * col) % 2 + (row * col) % 3) % 2 == 0; break;
                case 7: invert = ((row + col) % 2 + (row * col) % 3) % 2 == 0; break;
            }
            if (invert) {
                qrMatrix[row][col] ^= 1; // Invert the module
            }
        }
    }
}

// Format information table for error correction levels and masks
const uint16_t formatInfoTable[4][8] PROGMEM = {
    {0x77C4, 0x72F3, 0x7DAA, 0x789D, 0x662F, 0x6318, 0x6C41, 0x6976}, // Level L
    {0x5412, 0x5125, 0x5E7C, 0x5B4B, 0x45F9, 0x40CE, 0x4F97, 0x4AA0}, // Level M
    {0x355F, 0x3068, 0x3F31, 0x3A06, 0x24B4, 0x2183, 0x2EDA, 0x2BED}, // Level Q
    {0x1689, 0x13BE, 0x1CE7, 0x19D0, 0x0762, 0x0255, 0x0D0C, 0x083B}  // Level H
};

// Apply format information to QR matrix
void applyFormatInformation(uint8_t errorCorrectionLevel, uint8_t maskPattern, uint8_t size) {
    uint16_t formatInfo = pgm_read_word(&formatInfoTable[errorCorrectionLevel][maskPattern]);

    // Place format information around the timing patterns
    for (int i = 0; i < 15; i++) {
        bool bit = (formatInfo >> i) & 1;

        // Format info on the top-left (along and opposite timing patterns)
        if (i < 6) {
            qrMatrix[8][i] = bit;
            qrMatrix[i][8] = bit;
        } else if (i == 6) {
            qrMatrix[8][7] = bit;
            qrMatrix[7][8] = bit;
        } else {
            qrMatrix[8][14 - i] = bit;
            qrMatrix[14 - i][8] = bit;
        }

        // Mirror format info to opposite corners
        qrMatrix[size - 1 - i][8] = bit;
        qrMatrix[8][size - 1 - i] = bit;
    }
}

void generateQRCode(const char* text, uint8_t version, uint8_t errorCorrectionLevel) {
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

    // Step 6: Apply mask pattern
    uint8_t bestMaskPattern = 0; // Choose the best mask pattern based on tests
    applyMaskPattern(bestMaskPattern, pgm_read_byte(&qrVersions[version].size));

    // Step 7: Apply format information
    applyFormatInformation(errorCorrectionLevel, bestMaskPattern, pgm_read_byte(&qrVersions[version].size));

    // Step 8: Display matrix (for debugging)
    printQRMatrix(pgm_read_byte(&qrVersions[version].size));
}

void setup() {
    Serial.begin(115200);
    initGaloisField();
    generateQRCode("HELLO", 2, 1); // Example with version 2, error correction level M
}

void loop() {
    // Empty loop
}
