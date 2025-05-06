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
    // Return false if buffer is NULL
    if (buffer == NULL)
    {
        return false;
    }

    // Return false if length < 20
    if (len < 20)
    {
        return false;
    }

    // Return false if first 4 bytes are NOT the sync header 0x7F 0xF0 0x1C 0xAF

    // Copy bytes 4-7 into a uint32_t variable for packet count and use be32toh() to convert it from big-endian to host order

    // Copy bytes 8-11 into a float (x_rate) and use f_be32toh() to convert

    // Copy bytes 12-15 into y_rate and use f_be32toh() to convert

    // Copy bytes 16-19 into z_rate and use f_be32toh() to convert

    // Populate the `out` struct with those values

    // Return true

    /*
        NOTESS: 

        Probs use memcpy() or a union if not doing byte-by-byte extraction
        If I'm paranoid about alignment or strict aliasing, memcpy() is safest.
        Plug in my endian float helper since I wrote it already
    */

    return false;
}