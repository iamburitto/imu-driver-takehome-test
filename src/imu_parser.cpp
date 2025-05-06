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
    /*
        1. Check if buffer is NULL or length < 20
        - If so, return false

        2. Check if first 4 bytes are the sync header:
        - 0x7F, 0xF0, 0x1C, 0xAF
        - If not, return false

        3. Copy bytes 4–7 into a uint32_t variable for packet_count
        - Use be32toh() to convert it from big-endian to host order

        4. Copy bytes 8–11 into a float (x_rate)
        - Use f_be32toh() to convert

        5. Copy bytes 12–15 into y_rate
        - Also with f_be32toh()

        6. Copy bytes 16–19 into z_rate
        - Same thing

        7. Populate the `out` struct with those values

        8. Return true
    */

    return false;
}