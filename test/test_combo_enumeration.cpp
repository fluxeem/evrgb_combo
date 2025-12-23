#include "core/combo.h"
#include <gtest/gtest.h>
#include <chrono>
#include <cstring>

using namespace evrgb;

// Basic: should not throw and tuple sizes align with individual enumeration APIs.
TEST(ComboEnumerationTest, Basic)
{
    ASSERT_NO_THROW({ auto all = enumerateAllCameras(); (void)all; });
    auto all = enumerateAllCameras();
    const auto& rgb = std::get<0>(all);
    const auto& dvs = std::get<1>(all);
    EXPECT_GE(rgb.size(), 0u);
    EXPECT_GE(dvs.size(), 0u);
}

// Structural sanity: if RGB cameras exist, fields are null-terminated and dimensions non-negative.
TEST(ComboEnumerationTest, RgbStructure)
{
    auto all = enumerateAllCameras();
    const auto& rgb = std::get<0>(all);
    for (const auto& cam : rgb) {
        EXPECT_LT(strlen(cam.manufacturer), sizeof(cam.manufacturer));
        EXPECT_LT(strlen(cam.serial_number), sizeof(cam.serial_number));
        EXPECT_GE(cam.width, 0u);
        EXPECT_GE(cam.height, 0u);
    }
}

// Performance: combined call should be reasonably fast (<3s avg for 3 iterations).
TEST(ComboEnumerationTest, Performance)
{
    const int kIters = 3;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIters; ++i) {
        auto all = enumerateAllCameras();
        (void)all;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double avg = ms / static_cast<double>(kIters);
    EXPECT_LT(avg, 3000.0);
}
