#include "id_utils.h"

#include <array>
#include <iomanip>
#include <random>
#include <sstream>

namespace fountain {

std::string GenerateUuidV4() {
    std::array<unsigned char, 16> bytes {};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto &b : bytes) {
        b = static_cast<unsigned char>(dist(gen));
    }

    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        oss << std::setw(2) << static_cast<int>(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            oss << "-";
        }
    }
    return oss.str();
}

}  // namespace fountain
