#include <catch2/catch_test_macros.hpp> // REQUIRE, CHECK
#include <catch2/catch_approx.hpp>  // This is where Approx actually lives

#include "imu_parser.hpp"

using Catch::Approx;

/*
    If we assume network byte order (big endian), then code has to handle endianness like this:

    1. must convert from host byte order to network byte order (big endian) on the sending side
    2. must covert from network byte order (big endian) to host byte order on the receiving side
*/

TEST_CASE("Invalid IMU packet fails to parse")
{
    uint8_t buffer[20] =
    {
        0x7F, 0xF0, 0x1C, 0xAF, // Sync
        0x00, 0x00, 0x00, 0x02, // Packet count = 2
        0x41, 0x20, 0x00, 0x00, // X = 10.0f
        0x41, 0xA0, 0x00, 0x00, // Y = 20.0f
        0x42, 0x20, 0x00, 0x00  // Z = 40.0f
    };

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
            buffer[0] == 0x00;
        }

        SECTION("Bad sync word byte 1")
        {
            buffer[1] == 0x00;
        }

        SECTION("Bad sync word byte 2")
        {
            buffer[2] == 0x00;
        }

        SECTION("Bad sync word byte 3")
        {
            buffer[3] == 0x00;
        }

        REQUIRE(parse_imu_packet(&buffer[0], sizeof(buffer), pkt) == false);

    }
}

TEST_CASE("Valid IMU packet parses correctly")
{
    #pragma message("CATCH_VERSION: " CATCH_VERSION_STRING)

    uint8_t buffer[20] =
    {
        0x7F, 0xF0, 0x1C, 0xAF, // Sync
        0x00, 0x00, 0x00, 0x02, // Packet count = 2
        0x41, 0x20, 0x00, 0x00, // X = 10.0f
        0x41, 0xA0, 0x00, 0x00, // Y = 20.0f
        0x42, 0x20, 0x00, 0x00  // Z = 40.0f
    };

    ImuPacket pkt;

    REQUIRE(parse_imu_packet(&buffer[0], sizeof(buffer), pkt));
    REQUIRE(pkt.packet_count == 2);
    REQUIRE(pkt.x_rate == Approx(10.0f));
    REQUIRE(pkt.y_rate == Approx(20.0f));
    REQUIRE(pkt.z_rate == Approx(40.0f));

}