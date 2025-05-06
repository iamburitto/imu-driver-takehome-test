#include "imu_parser.hpp"

/**
 * Parses a 20-byte IMU packet buffer.
 * @param buffer Pointer to at least 20 bytes of data, starting at sync header.
 * @param len Number of bytes available (should be >= 20).
 * @param out Reference to store parsed result.
 * @return true if successfully parsed; false if invalid or too short.
 */
bool parse_imu_packet(const uint8_t* buffer, size_t len, ImuPacket& out)
{
    return false;
}