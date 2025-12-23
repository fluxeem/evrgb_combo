/**
 * @file test_combo_dvs_callback.cpp
 * @brief Test sample for DVS event callback interface in Combo class.
 * 
 * Purpose:
 * Verify that the DVS event callback can be successfully registered, 
 * receives events, and can be unregistered.
 * 
 * Implementation:
 * 1. Initialize Combo device (automatically selects available cameras).
 * 2. Register a callback function `onDvsEvents` to count DVS events.
 * 3. Start the device and run for 3 seconds.
 * 4. Print the total number of events received.
 * 5. Remove the callback and stop the device.
 */

#include "core/combo.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>

// Global counter
std::atomic<uint64_t> g_event_count{0};

// Callback function
// Signature: void(dvsense::Event2D* begin, dvsense::Event2D* end)
void onDvsEvents(dvsense::Event2D* begin, dvsense::Event2D* end) {
    if (begin && end && end >= begin) {
        g_event_count += (end - begin);
    }
}

int main() {
    std::cout << "Test Combo DVS Callback" << std::endl;

    // Enumerate available cameras to find serials (simplified logic)
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    
    std::string rgb_serial;
    for (const auto& info : rgb_cameras) {
        if (std::strcmp(info.manufacturer, "Hikrobot") == 0) {
            rgb_serial = info.serial_number;
            break;
        }
    }
    if (rgb_serial.empty() && !rgb_cameras.empty()) {
        rgb_serial = rgb_cameras[0].serial_number;
    }
    std::string dvs_serial = dvs_cameras.empty() ? "" : dvs_cameras[0].serial;

    evrgb::Combo combo(rgb_serial, dvs_serial);
    
    if (!combo.init()) {
        std::cerr << "Init failed" << std::endl;
        return -1;
    }

    // Add callback
    uint32_t cb_id = combo.addDvsEventCallback(onDvsEvents);
    std::cout << "Callback added with ID: " << cb_id << std::endl;

    if (!combo.start()) {
        std::cerr << "Start failed" << std::endl;
        return -1;
    }

    std::cout << "Running for 3 seconds..." << std::endl;

    for (int i = 0; i < 25; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "Elapsed: " << (i + 1) << " s, Events received: " << g_event_count.load() << std::endl;
    }
    

    std::cout << "Total events received: " << g_event_count.load() << std::endl;

    if (combo.removeDvsEventCallback(cb_id)) {
        std::cout << "Callback removed." << std::endl;
    } else {
        std::cerr << "Failed to remove callback." << std::endl;
    }
    
    combo.stop();
    combo.destroy();

    std::cout << "Test finished." << std::endl;

    return 0;
}

