/// This software is distributed under the terms of the MIT License.
/// Copyright (c) 2020 UAVCAN Development Team.
/// Author: Pavel Kirienko <pavel.kirienko@zubax.com>

#include "socketcan.h"
#include "catch.hpp"
#include <cstring>
#include <unistd.h>

TEST_CASE("IO-Classic")  // Catch2 does not support parametrized tests yet.
{
    const char* const iface_name = "vcan0";

    const auto sa = socketcanOpen(iface_name, false);
    const auto sb = socketcanOpen(iface_name, false);
    REQUIRE(sa >= 0);
    REQUIRE(sb >= 0);

    CanardFrame fr{};
    fr.extended_can_id = 0x1234U;
    fr.payload_size    = 6;
    fr.payload         = "Hello";
    REQUIRE(1 == socketcanPush(sa, &fr, 1'000'000));

    fr.extended_can_id = 0x4321U;
    fr.payload_size    = 7;
    fr.payload         = "World!";
    REQUIRE(1 == socketcanPush(sb, &fr, 0));

    char buf[255]{};
    fr = {};
    REQUIRE(1 == socketcanPop(sb, &fr, sizeof(buf), buf, 1000));
    REQUIRE(fr.timestamp_usec > 0);
    REQUIRE(fr.extended_can_id == 0x1234U);
    REQUIRE(fr.payload_size == 6);
    REQUIRE(0 == std::memcmp(fr.payload, "Hello", 6));
    auto old_ts = fr.timestamp_usec;

    fr = {};
    REQUIRE(1 == socketcanPop(sa, &fr, sizeof(buf), buf, 1000));
    REQUIRE(fr.timestamp_usec > 0);
    REQUIRE(fr.timestamp_usec >= old_ts);
    REQUIRE(fr.extended_can_id == 0x4321U);
    REQUIRE(fr.payload_size == 7);
    REQUIRE(0 == std::memcmp(fr.payload, "World!", 7));

    REQUIRE(0 == socketcanPop(sa, &fr, sizeof(buf), buf, 0));
    REQUIRE(0 == socketcanPop(sa, &fr, sizeof(buf), buf, 1000));
    REQUIRE(-EINVAL == socketcanPop(sa, &fr, 0, nullptr, 1000));
    REQUIRE(-EINVAL == socketcanPop(sa, nullptr, sizeof(buf), buf, 1000));

    REQUIRE(-EINVAL == socketcanPush(sa, nullptr, 1'000'000));

    ::close(sa);
    ::close(sb);
}

TEST_CASE("IO-FD")
{
    const char* const iface_name = "vcan0";

    const auto sa = socketcanOpen(iface_name, true);
    const auto sb = socketcanOpen(iface_name, true);
    REQUIRE(sa >= 0);
    REQUIRE(sb >= 0);

    const SocketCANFilterConfig fcs = {
        0x00001234U,
        0x1FFFFFFFU,
    };
    REQUIRE(0 == socketcanFilter(sb, 1, &fcs));
    REQUIRE(-EFBIG == socketcanFilter(sa, 1'000'000, &fcs));
    REQUIRE(-EINVAL == socketcanFilter(sa, 1, NULL));

    CanardFrame fr{};
    fr.extended_can_id = 0x1234U;
    fr.payload_size    = 13;
    fr.payload         = "Hello world!";
    REQUIRE(1 == socketcanPush(sa, &fr, 1'000'000));

    fr.extended_can_id = 0x4321U;
    fr.payload_size    = 10;
    fr.payload         = "0123456789";
    REQUIRE(1 == socketcanPush(sb, &fr, 0));

    char buf[255]{};
    fr = {};
    REQUIRE(1 == socketcanPop(sb, &fr, sizeof(buf), buf, 1000));
    REQUIRE(fr.timestamp_usec > 0);
    REQUIRE(fr.extended_can_id == 0x1234U);
    REQUIRE(fr.payload_size == 13);
    REQUIRE(0 == std::memcmp(fr.payload, "Hello world!", 13));
    auto old_ts = fr.timestamp_usec;

    fr = {};
    REQUIRE(1 == socketcanPop(sa, &fr, sizeof(buf), buf, 1000));
    REQUIRE(fr.timestamp_usec > 0);
    REQUIRE(fr.timestamp_usec >= old_ts);
    REQUIRE(fr.extended_can_id == 0x4321U);
    REQUIRE(fr.payload_size == 10);
    REQUIRE(0 == std::memcmp(fr.payload, "0123456789", 10));

    REQUIRE(0 == socketcanPop(sa, &fr, sizeof(buf), buf, 0));
    REQUIRE(0 == socketcanPop(sa, &fr, sizeof(buf), buf, 1000));
    REQUIRE(-EINVAL == socketcanPop(sa, &fr, 0, nullptr, 1000));
    REQUIRE(-EINVAL == socketcanPop(sa, nullptr, sizeof(buf), buf, 1000));

    REQUIRE(-EINVAL == socketcanPush(sa, nullptr, 1'000'000));

    ::close(sa);
    ::close(sb);
}
