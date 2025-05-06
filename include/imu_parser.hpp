#ifndef IMU_PARSER_H
#define IMU_PARSER_H

#include <cstddef>
#include <cstdio>
#include "fendian.hpp"

/*
    If we assume network byte order (big endian), then code has to handle endianness like this:

    1. must convert from host byte order to network byte order (big endian) on the sending side
    2. must covert from network byte order (big endian) to host byte order on the receiving side
*/

struct ImuPacket {
    uint32_t packet_count;
    float32_t x_rate;
    float32_t y_rate;
    float32_t z_rate;
};

bool parse_imu_packet(const uint8_t* buffer, size_t len, ImuPacket& out);

#endif /* IMU_PARSER_H */