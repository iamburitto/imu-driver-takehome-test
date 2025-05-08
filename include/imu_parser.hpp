#ifndef IMU_PARSER_H
#define IMU_PARSER_H

#include <cstddef>
#include <cstdio>
#include "fendian.hpp"

// TODO: do I need to pack this? probably not since they're all the same data size.
typedef struct
{
    uint32_t packet_count;
    float32_t x_rate_rdps;
    float32_t y_rate_rdps;
    float32_t z_rate_rdps;
} imu_packet_t;

bool parse_imu_packet(const uint8_t* buffer, size_t len, imu_packet_t& out);

#endif /* IMU_PARSER_H */