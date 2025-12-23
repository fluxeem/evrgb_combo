#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include "core/combo.h"

int main() {
    std::cout << "Test Combo Synced Callback" << std::endl;

    // Enumerate available cameras to find serials (simplified logic)
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    
    std::string rgb_serial;
    std::string dvs_serial;

    if (!rgb_cameras.empty()) {
        rgb_serial = rgb_cameras[0].serial_number;
        std::cout << "Found RGB Camera: " << rgb_serial << std::endl;
    } else {
        std::cerr << "No RGB camera found!" << std::endl;
        return -1;
    }

    if (!dvs_cameras.empty()) {
        dvs_serial = dvs_cameras[0].serial;
        std::cout << "Found DVS Camera: " << dvs_serial << std::endl;
    } else {
        std::cerr << "No DVS camera found!" << std::endl;
        return -1;
    }

    // Create Combo instance
    evrgb::Combo combo(rgb_serial, dvs_serial);

    // Initialize
    if (!combo.init()) {
        std::cerr << "Failed to initialize Combo" << std::endl;
        return -1;
    }

    // Set synchronized callback
    combo.setSyncedCallback([](const evrgb::RgbImageWithTimestamp& img_data, const std::vector<dvsense::Event2D>& events) {
        // This callback runs in a separate thread
        std::cout << "[Synced Callback] Image Index: " << img_data.image_index 
                  << ", Exposure: [" << img_data.exposure_start_ts << " - " << img_data.exposure_end_ts << "]"
                  << ", Event Count: " << events.size() << std::endl;
        
        // Simulate some processing time to test non-blocking behavior
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    // Start
    if (!combo.start()) {
        std::cerr << "Failed to start Combo" << std::endl;
        return -1;
    }

    std::cout << "Combo started. Press Ctrl+C to stop." << std::endl;

    // Main loop
    std::this_thread::sleep_for(std::chrono::seconds(5));


    // Stop and destroy
    combo.stop();
    combo.destroy();

    std::cout << "Test finished." << std::endl;

    return 0;
}

