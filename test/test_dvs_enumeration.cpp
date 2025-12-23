#include "camera/dvs_camera.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace evrgb;

// Basic test: enumerate should not throw and return a vector (possibly empty)
TEST(DvsCameraTest, BasicEnumeration)
{
    ASSERT_NO_THROW({
        auto dvs = enumerateAllDvsCameras();
        (void)dvs;
    });

    auto dvs = enumerateAllDvsCameras();
    // Just verify it's a valid container; size may be zero if no devices.
    EXPECT_GE(dvs.size(), 1u);
}

// Optional: quick performance sanity (< 2s per call on avg for 3 calls)
TEST(DvsCameraTest, EnumerationPerformance)
{
    const int kIters = 3;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIters; ++i) {
        auto dvs = enumerateAllDvsCameras();
        (void)dvs;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double avg = ms / static_cast<double>(kIters);
    EXPECT_LT(avg, 2000.0);
}
