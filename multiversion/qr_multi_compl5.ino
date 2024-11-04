// Rule 1: Evaluate rows and columns for consecutive modules of the same color
int penaltyRule1(uint8_t size) {
    int penalty = 0;

    // Check rows
    for (int row = 0; row < size; row++) {
        int consecutiveCount = 1;
        for (int col = 1; col < size; col++) {
            if (qrMatrix[row][col] == qrMatrix[row][col - 1]) {
                consecutiveCount++;
            } else {
                if (consecutiveCount >= 5) {
                    penalty += 3 + (consecutiveCount - 5);
                }
                consecutiveCount = 1;
            }
        }
        if (consecutiveCount >= 5) {
            penalty += 3 + (consecutiveCount - 5);
        }
    }

    // Check columns
    for (int col = 0; col < size; col++) {
        int consecutiveCount = 1;
        for (int row = 1; row < size; row++) {
            if (qrMatrix[row][col] == qrMatrix[row - 1][col]) {
                consecutiveCount++;
            } else {
                if (consecutiveCount >= 5) {
                    penalty += 3 + (consecutiveCount - 5);
                }
                consecutiveCount = 1;
            }
        }
        if (consecutiveCount >= 5) {
            penalty += 3 + (consecutiveCount - 5);
        }
    }
    return penalty;
}

// Rule 2: Evaluate 2x2 blocks of the same color
int penaltyRule2(uint8_t size) {
    int penalty = 0;
    for (int row = 0; row < size - 1; row++) {
        for (int col = 0; col < size - 1; col++) {
            if (qrMatrix[row][col] == qrMatrix[row][col + 1] &&
                qrMatrix[row][col] == qrMatrix[row + 1][col] &&
                qrMatrix[row][col] == qrMatrix[row + 1][col + 1]) {
                penalty += 3;
            }
        }
    }
    return penalty;
}

// Rule 3: Patterns resembling finder patterns (1:1:3:1:1) in rows or columns
int penaltyRule3(uint8_t size) {
    int penalty = 0;
    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size - 6; col++) {
            if ((qrMatrix[row][col] == 1 && qrMatrix[row][col + 1] == 0 &&
                 qrMatrix[row][col + 2] == 1 && qrMatrix[row][col + 3] == 1 &&
                 qrMatrix[row][col + 4] == 1 && qrMatrix[row][col + 5] == 0 &&
                 qrMatrix[row][col + 6] == 1) ||
                (qrMatrix[col][row] == 1 && qrMatrix[col + 1][row] == 0 &&
                 qrMatrix[col + 2][row] == 1 && qrMatrix[col + 3][row] == 1 &&
                 qrMatrix[col + 4][row] == 1 && qrMatrix[col + 5][row] == 0 &&
                 qrMatrix[col + 6][row] == 1)) {
                penalty += 40;
            }
        }
    }
    return penalty;
}

// Rule 4: Evaluate dark module balance
int penaltyRule4(uint8_t size) {
    int darkModules = 0;
    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
            if (qrMatrix[row][col] == 1) darkModules++;
        }
    }
    int totalModules = size * size;
    int percentDark = (darkModules * 100) / totalModules;
    int deviation = abs(percentDark - 50) / 5;
    return deviation * 10;
}
uint8_t selectBestMaskPattern(uint8_t size) {
    int bestScore = INT_MAX;
    uint8_t bestPattern = 0;

    for (uint8_t pattern = 0; pattern < 8; pattern++) {
        applyMaskPattern(pattern, size); // Apply the mask

        int score = penaltyRule1(size) + penaltyRule2(size) + penaltyRule3(size) + penaltyRule4(size);

        if (score < bestScore) {
            bestScore = score;
            bestPattern = pattern;
        }

        applyMaskPattern(pattern, size); // Undo the mask for the next iteration
    }
    return bestPattern;
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

    // Step 6: Choose the best mask pattern
    uint8_t bestMaskPattern = selectBestMaskPattern(pgm_read_byte(&qrVersions[version].size));
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
