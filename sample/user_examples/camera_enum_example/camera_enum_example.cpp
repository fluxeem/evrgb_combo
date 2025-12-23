#include "core/combo.h"
#include <iostream>

int main() {
    std::cout << "EvRGB Combo SDK Sample - Camera Enumeration Example" << std::endl;
    auto [rgb_camera_infos, dvs_camera_infos] = evrgb::enumerateAllCameras();

    std::cout << "Found " << rgb_camera_infos.size() << " RGB cameras:" << std::endl;
    for (const auto& info : rgb_camera_infos) {
        std::cout << "  Manufacturer: " << info.manufacturer << ", Serial: " << info.serial_number
                  << ", Resolution: " << info.width << "x" << info.height << std::endl;
    }
    std::cout << "Found " << dvs_camera_infos.size() << " DVS cameras:" << std::endl;
    for (const auto& info : dvs_camera_infos) {
        std::cout << "  Manufacturer: " << info.manufacturer << ", Serial: " << info.serial << std::endl;
    }

    evrgb::Combo combo(rgb_camera_infos.empty() ? "" : rgb_camera_infos[0].serial_number,
                dvs_camera_infos.empty() ? "" : dvs_camera_infos[0].serial);

    return 0;
}
