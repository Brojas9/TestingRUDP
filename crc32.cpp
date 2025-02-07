#include "crc32.h"
#include <fstream>
#include <iostream>

// Friesen, B. (2001, December 17). CRC32: Generating a checksum for a file. 
// CodeProject. https://www.codeproject.com/Articles/1671/CRC32-Generating-a-checksum-for-a-file

static uint32_t crcTable[256];

// Generate CRC-32 Lookup Table
void generateCRCTable() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crcTable[i] = crc;
    }
}

// Compute CRC-32 for a data block
uint32_t CRC32::compute(const std::vector<uint8_t>& data) {
    static bool tableGenerated = false;
    if (!tableGenerated) {
        generateCRCTable();
        tableGenerated = true;
    }

    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc = (crc >> 8) ^ crcTable[(crc ^ byte) & 0xFF];
    }

    return crc ^ 0xFFFFFFFF;
}

// Compute CRC-32 for an entire file
uint32_t CRC32::computeFile(const std::string& filePath) {
    static bool tableGenerated = false;
    if (!tableGenerated) {
        generateCRCTable();
        tableGenerated = true;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file for CRC computation.\n";
        return 0;
    }

    uint32_t crc = 0xFFFFFFFF;
    std::vector<uint8_t> buffer(4096);
    while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || file.gcount() > 0) {
        size_t bytesRead = file.gcount();
        for (size_t i = 0; i < bytesRead; ++i) {
            crc = (crc >> 8) ^ crcTable[(crc ^ buffer[i]) & 0xFF];
        }
    }

    return crc ^ 0xFFFFFFFF;
}
