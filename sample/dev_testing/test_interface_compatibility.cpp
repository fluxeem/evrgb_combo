#include "core/combo.h"
#include "camera/rgb_camera.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Testing RGB Camera interface compatibility..." << std::endl;

    // Test 1: Enumerate cameras
    std::cout << "\n1. Testing camera enumeration..." << std::endl;
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    std::cout << "Found " << rgb_cameras.size() << " RGB cameras" << std::endl;
    for (const auto& cam : rgb_cameras) {
        std::cout << "  - Serial: " << cam.serial_number 
                  << ", Manufacturer: " << cam.manufacturer
                  << ", Resolution: " << cam.width << "x" << cam.height << std::endl;
    }

    // Test 2: Test interface usage with Hikvision camera
    std::cout << "\n2. Testing interface compatibility..." << std::endl;
    
    std::string test_serial = rgb_cameras.empty() ? "" : rgb_cameras[0].serial_number;
    std::cout << "Using RGB camera with serial: " << test_serial << std::endl;
    
    // Create HikvisionRgbCamera instance
    std::shared_ptr<evrgb::HikvisionRgbCamera> hikvision_camera = 
        std::make_shared<evrgb::HikvisionRgbCamera>();
    
    // Test using interface pointer
    std::shared_ptr<evrgb::IRgbCamera> rgb_interface = hikvision_camera;
    
    if (!test_serial.empty()) {
        // Test initialization through interface
        std::cout << "Initializing camera through interface..." << std::endl;
        if (rgb_interface->initialize(test_serial)) {
            std::cout << "Camera initialized successfully" << std::endl;
            std::cout << "Camera dimensions: " << rgb_interface->getWidth() 
                      << "x" << rgb_interface->getHeight() << std::endl;
            
            // Test property access through interface
            std::cout << "Testing property access..." << std::endl;
            rgb_interface->setProperty("ExposureTime", 10000.0); // 10ms exposure
            double exp_time = rgb_interface->getPropertyDouble("ExposureTime");
            std::cout << "Exposure time: " << exp_time << std::endl;
            
            // Test starting camera
            std::cout << "Starting camera..." << std::endl;
            if (rgb_interface->start()) {
                std::cout << "Camera started successfully" << std::endl;
                
                // Test capturing an image
                cv::Mat image;
                std::cout << "Attempting to capture an image..." << std::endl;
                if (rgb_interface->getLatestImage(image)) {
                    std::cout << "Image captured successfully: " 
                              << image.cols << "x" << image.rows 
                              << ", channels: " << image.channels() << std::endl;
                } else {
                    std::cout << "Failed to capture image (may be expected if no camera connected)" << std::endl;
                }
                
                // Test stopping camera
                std::cout << "Stopping camera..." << std::endl;
                if (rgb_interface->stop()) {
                    std::cout << "Camera stopped successfully" << std::endl;
                } else {
                    std::cout << "Failed to stop camera" << std::endl;
                }
            } else {
                std::cout << "Failed to start camera" << std::endl;
            }
        } else {
            std::cout << "Failed to initialize camera" << std::endl;
        }
        
        // Cleanup
        std::cout << "Destroying camera..." << std::endl;
        rgb_interface->destroy();
        std::cout << "Camera destroyed" << std::endl;
    } else {
        std::cout << "No RGB cameras found to test with" << std::endl;
    }

    // Test 3: Test Combo class with new interface
    std::cout << "\n3. Testing Combo class with new interface..." << std::endl;
    evrgb::Combo combo(test_serial, ""); // Only RGB, no DVS for this test
    
    if (!test_serial.empty()) {
        if (combo.init()) {
            std::cout << "Combo initialized successfully" << std::endl;
            
            // Get RGB camera interface from combo
            auto combo_rgb_camera = combo.getRgbCamera();
            if (combo_rgb_camera) {
                std::cout << "Retrieved RGB camera interface from Combo" << std::endl;
                std::cout << "Combo RGB camera dimensions: " 
                          << combo_rgb_camera->getWidth() << "x" 
                          << combo_rgb_camera->getHeight() << std::endl;
            } else {
                std::cout << "Failed to retrieve RGB camera interface from Combo" << std::endl;
            }
            
            combo.destroy();
            std::cout << "Combo destroyed" << std::endl;
        } else {
            std::cout << "Failed to initialize Combo" << std::endl;
        }
    }

    std::cout << "\nInterface compatibility test completed!" << std::endl;
    return 0;
}