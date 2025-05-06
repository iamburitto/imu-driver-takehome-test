#include <catch2/catch_test_macros.hpp> // REQUIRE, CHECK
#include <catch2/catch_approx.hpp>  // This is where Approx actually lives

#include "imu_parser.hpp"

using Catch::Approx;

/*
    If we assume network byte order (big endian), then I think code has to handle endianness like this:

    1. must convert from host byte order to network byte order (big endian) on the sending side
    2. must covert from network byte order (big endian) to host byte order on the receiving side
*/

TEST_CASE("Invalid IMU packet fails")
{
    uint8_t buffer[20] = {0};

    // Sync header
    buffer[0] = 0x7F;
    buffer[1] = 0xF0;
    buffer[2] = 0x1C;
    buffer[3] = 0xAF;

    // packet count and gyros in host-endian
    uint32_t packet_count = 2;
    float32_t x = 10.0f;
    float32_t y = 20.0f;
    float32_t z = 40.0f;

    ImuPacket pkt;

    SECTION("NULL buffer")
    {
        REQUIRE(parse_imu_packet(NULL, sizeof(buffer), pkt) == false);
    }

    SECTION("Invalid length (not enough bytes)")
    {
        REQUIRE(parse_imu_packet(&buffer[0], 19, pkt) == false);
    }
    
    SECTION("Bad SYNC word")
    {
        SECTION("Bad sync word byte 0")
        {
            buffer[0] = 0x00;
        }

        SECTION("Bad sync word byte 1")
        {
            buffer[1] = 0x00;
        }

        SECTION("Bad sync word byte 2")
        {
            buffer[2] = 0x00;
        }

        SECTION("Bad sync word byte 3")
        {
            buffer[3] = 0x00;
        }

        REQUIRE(parse_imu_packet(&buffer[0], sizeof(buffer), pkt) == false);
    }
}

TEST_CASE("Valid IMU packet parses correctly")
{
    uint8_t buffer[20] = {0};

    // Sync header
    buffer[0] = 0x7F;
    buffer[1] = 0xF0;
    buffer[2] = 0x1C;
    buffer[3] = 0xAF;

    // packet count and gyros in host-endian
    uint32_t packet_count = 2;
    float32_t x = 10.0f;
    float32_t y = 20.0f;
    float32_t z = 40.0f;

    // Packet count in big-endian
    uint32_t packet_count_be = htobe32(packet_count);
    // Gyro floats in big-endian using my f_htobe32() helper
    float32_t x_be = f_htobe32(x);
    float32_t y_be = f_htobe32(y);
    float32_t z_be = f_htobe32(z);

    memcpy(&buffer[4],  &packet_count_be, sizeof(uint32_t));
    memcpy(&buffer[8],  &x_be, sizeof(float32_t));
    memcpy(&buffer[12], &y_be, sizeof(float32_t));
    memcpy(&buffer[16], &z_be, sizeof(float32_t));

    ImuPacket pkt;

    REQUIRE(parse_imu_packet(buffer, sizeof(buffer), pkt));
    REQUIRE(pkt.packet_count == packet_count);
    REQUIRE(pkt.x_rate == Approx(x));
    REQUIRE(pkt.y_rate == Approx(y));
    REQUIRE(pkt.z_rate == Approx(z));
}