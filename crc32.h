#ifndef CRC32_H
#define CRC32_H

#include <vector>
#include <string>

class CRC32 {
public:
    static uint32_t compute(const std::vector<uint8_t>& data);
    static uint32_t computeFile(const std::string& filePath);
};

#endif // CRC32_H
