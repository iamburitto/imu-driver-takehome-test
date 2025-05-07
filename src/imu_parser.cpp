#include "imu_parser.hpp"

/*
    If we assume network byte order (big endian), then I think code has to handle endianness like this:

    1. must convert from host byte order to network byte order (big endian) on the sending side
    2. must covert from network byte order (big endian) to host byte order on the receiving side
*/

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
    if ((buffer[0] != 0x7F) || (buffer[1] != 0xF0) || (buffer[2] != 0x1C) || (buffer[3] != 0xAF))
    {
        return false;
    }

    printf("buffer[0]: %02X ", buffer[0]);
    printf("buffer[1]: %02X ", buffer[1]);
    printf("buffer[2]: %02X ", buffer[2]);
    printf("buffer[3]: %02X ", buffer[3]);
    printf("\n");

    // TODO: Define offsets instead of magic numbers

    // Copy bytes 4-7 into a uint32_t variable for packet count and use be32toh() to convert it from big-endian to host order
    uint32_t packet_count_be;
    memcpy(&packet_count_be, &buffer[4], sizeof(uint32_t));
    out.packet_count = be32toh(packet_count_be);
    // ? Do I need to worry if the actual packet count matches the one in the packet?

    // Floats are assumed to be in IEEE-754 network byte order (big-endian)

    // Copy bytes 8-11 into a float (x_rate) and use f_be32toh() to convert
    float32_t x_rate_be;
    memcpy(&x_rate_be, &buffer[8], sizeof(float32_t));
    out.x_rate_rdps = f_be32toh(x_rate_be);

    // Copy bytes 12-15 into y_rate and use f_be32toh() to convert
    float32_t y_rate_be;
    memcpy(&y_rate_be, &buffer[12], sizeof(float32_t));
    out.y_rate_rdps = f_be32toh(y_rate_be);

    // Copy bytes 16-19 into z_rate and use f_be32toh() to convert
    float32_t z_rate_be;
    memcpy(&z_rate_be, &buffer[16], sizeof(float32_t));
    out.z_rate_rdps = f_be32toh(z_rate_be);

    // Return true since there's no way to calculate a checksum
    return true;
}