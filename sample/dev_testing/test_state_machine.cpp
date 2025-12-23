#include "utils/evrgb_logger.h"
#include "camera/rgb_camera.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace evrgb;

void printState(const HikvisionRgbCamera& camera, const std::string& context)
{
    std::cout << "[" << context << "] Camera state: " 
              << static_cast<int>(camera.getState()) 
              << ", Connected: " << (camera.isConnected() ? "Yes" : "No") << std::endl;
}

int main()
{
    // 初始化日志系统
    set_log_level(evrgb::LogLevel::debug);
    
    std::cout << "\n=== Testing HikvisionRgbCamera State Machine ===" << std::endl;
    
    // 测试1: 默认构造函数
    std::cout << "\n1. Testing default constructor:" << std::endl;
    {
        HikvisionRgbCamera camera;
        printState(camera, "After construction");
        
        // 测试初始化
        std::cout << "\nTesting initialize()..." << std::endl;
        if (camera.initialize()) {
            printState(camera, "After init");
            
            // 测试启动
            std::cout << "\nTesting start()..." << std::endl;
            if (camera.start()) {
                printState(camera, "After start");
                
                // 等待一会儿
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                // 测试停止
                std::cout << "\nTesting stop()..." << std::endl;
                if (camera.stop()) {
                    printState(camera, "After stop");
                }
            }
            
            // 测试销毁
            std::cout << "\nTesting destroy()..." << std::endl;
            camera.destroy();
            printState(camera, "After destroy");
        } else {
            std::cout << "Failed to initialize camera" << std::endl;
        }
    }
    
    // 测试2: 序列号构造函数
    std::cout << "\n2. Testing serial number constructor:" << std::endl;
    auto cameras = enumerateAllRgbCameras();
    if (!cameras.empty()) {
        std::string serialNumber = cameras[0].serial_number;
        std::cout << "Using serial number: " << serialNumber << std::endl;
        
        HikvisionRgbCamera camera(serialNumber);
        printState(camera, "After construction with serial");
        
        if (camera.getState() == HikvisionRgbCamera::CameraState::INITIALIZED) {
            std::cout << "\nTesting start()..." << std::endl;
            if (camera.start()) {
                printState(camera, "After start");
                
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                std::cout << "\nTesting stop()..." << std::endl;
                camera.stop();
                printState(camera, "After stop");
            }
        }
    } else {
        std::cout << "No cameras found for serial number test" << std::endl;
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}

