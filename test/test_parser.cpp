#include <catch2/catch_test_macros.hpp>
#include "imu_parser.hpp"

TEST_CASE("Valid IMU packet parses correctly")
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
    REQUIRE(parse_imu_packet(&buffer[0], sizeof(buffer), pkt));
    REQUIRE(pkt.packet_count == 2);
    REQUIRE(pkt.x_rate == Approx(10.0f));
    REQUIRE(pkt.y_rate == Approx(20.0f));
    REQUIRE(pkt.z_rate == Approx(40.0f));
}