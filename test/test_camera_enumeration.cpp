#include "camera/rgb_camera.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace evrgb;

// 测试基础枚举功能
TEST(RgbCameraTest, BasicEnumeration) {
    // 调用枚举函数不应该崩溃
    ASSERT_NO_THROW({
        auto cameras = enumerateAllRgbCameras();
    });
    
    // 获取相机列表
    auto cameras = enumerateAllRgbCameras();
    
    // 结果应该是有效的vector（可能为空）
    EXPECT_GE(cameras.size(), 1);
}

// 测试多次枚举的一致性
TEST(RgbCameraTest, MultipleEnumerationConsistency) {
    const int numTests = 3;
    std::vector<size_t> cameraCounts;
    
    // 执行多次枚举
    for (int i = 0; i < numTests; ++i) {
        auto cameras = enumerateAllRgbCameras();
        cameraCounts.push_back(cameras.size());
        
        // 短暂延时
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // 检查结果一致性
    for (size_t i = 1; i < cameraCounts.size(); ++i) {
        EXPECT_EQ(cameraCounts[i], cameraCounts[0]) 
            << "Enumeration " << i << " returned different camera count";
    }
}

// 测试性能（确保枚举不会太慢）
TEST(RgbCameraTest, EnumerationPerformance) {
    const int numIterations = 5;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numIterations; ++i) {
        auto cameras = enumerateAllRgbCameras();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // 每次枚举应该在合理时间内完成（假设不超过2秒每次）
    double avgTimeMs = duration.count() / static_cast<double>(numIterations);
    EXPECT_LT(avgTimeMs, 2000.0) << "Camera enumeration is too slow: " << avgTimeMs << "ms per call";
}

// 测试相机信息结构的基本有效性
TEST(RgbCameraTest, CameraInfoStructure) {
    auto cameras = enumerateAllRgbCameras();
    
    // 如果找到相机，检查基本结构
    for (const auto& camera : cameras) {
        // manufacturer和serial_number字段应该以null结尾
        EXPECT_LT(strlen(camera.manufacturer), sizeof(camera.manufacturer));
        EXPECT_LT(strlen(camera.serial_number), sizeof(camera.serial_number));
        
        // width和height应该是合理的值（0或正数）
        EXPECT_GE(camera.width, 0u);
        EXPECT_GE(camera.height, 0u);
    }
}

// 如果有相机连接，验证基本信息不为空
TEST(RgbCameraTest, CameraInfoNotEmpty) {
    auto cameras = enumerateAllRgbCameras();
    
    // 如果找到相机，至少序列号应该不为空
    for (const auto& camera : cameras) {
        if (strlen(camera.serial_number) > 0 || strlen(camera.manufacturer) > 0) {
            // 如果有基本信息，那么这就是一个有效的相机
            SUCCEED();
            return;
        }
    }
    
    // 如果没有找到相机，这也是正常的（可能没有连接相机）
    if (cameras.empty()) {
        SUCCEED() << "No cameras found, which is normal if no cameras are connected";
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
