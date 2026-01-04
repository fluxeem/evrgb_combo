#include "core/combo.h"

#include <iostream>
#include <memory>
#include <string>

int main() {
    std::cout << "Device model name test" << std::endl;

    // Enumerate both camera types first so we know what to test.
    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    std::cout << "Found " << rgb_cameras.size() << " RGB cameras, "
              << dvs_cameras.size() << " DVS cameras" << std::endl;

    const std::string rgb_serial = rgb_cameras.empty() ? "" : rgb_cameras[0].serial_number;
    const std::string dvs_serial = dvs_cameras.empty() ? "" : dvs_cameras[0].serial;

    evrgb::Combo combo(rgb_serial, dvs_serial);
    if (!combo.init()) {
        std::cerr << "Combo initialization failed" << std::endl;
        return 1;
    }

    // --- RGB camera: test getDeviceModelName(StringProperty&) through Combo ---
    if (auto rgb_cam = combo.getRgbCamera()) {
        std::cout << "\nTesting RGB camera (serial=" << rgb_serial << ")" << std::endl;

        evrgb::StringProperty model_prop;
        auto rgb_interface = combo.getRgbCamera();
        evrgb::CameraStatus status = rgb_interface->getDeviceModelName(model_prop);
        if (status.success()) {
            std::cout << "Model: " << model_prop.value
                      << " (max_len=" << model_prop.max_len << ")" << std::endl;
        } else {
            std::cerr << "RGB getDeviceModelName failed, code=" << status.code
                      << ", msg=" << status.message << std::endl;
        }
    } else {
        std::cout << "\nRGB camera not available from Combo" << std::endl;
    }

    // --- DVS camera: test getDeviceModelName(std::string&) through Combo ---
    {
        std::string model_name;

        auto dvs_interface = combo.getDvsCamera();
        std::string dvs_model_name = dvs_interface->getDescription().product;

        std::cout << "\nTesting DVS camera (serial=" << dvs_serial << ")" << std::endl;
        if (!dvs_model_name.empty()) {
            std::cout << "Model: " << dvs_model_name << std::endl;
        } else {
            std::cerr << "DVS getDeviceModelName failed or returned empty name" << std::endl;
        }
    }

    combo.destroy();

    std::cout << "\nDone." << std::endl;
    return 0;
}
