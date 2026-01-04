// Copyright (c) 2025.
// Shared RGB camera factory and enumeration utilities.

#include "camera/i_camera.h"
#include "utils/evrgb_logger.h"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <vector>

namespace evrgb {
namespace {

struct FactoryRegistry {
    std::mutex mutex;
    std::vector<std::pair<std::string, RgbCameraFactoryFn>> factories;
    std::vector<RgbEnumeratorFn> enumerators;
};

FactoryRegistry& getRegistry()
{
    static FactoryRegistry registry;
    return registry;
}

std::string toLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

RgbCameraFactoryFn findFactory(const std::string& manufacturer)
{
    auto& registry = getRegistry();
    std::lock_guard<std::mutex> lock(registry.mutex);

    const std::string manufacturer_lower = toLowerCopy(manufacturer);
    for (const auto& entry : registry.factories) {
        if (manufacturer_lower.find(entry.first) != std::string::npos) {
            return entry.second;
        }
    }
    return {};
}

std::vector<RgbEnumeratorFn> getEnumerators()
{
    auto& registry = getRegistry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    return registry.enumerators; // copy for thread safety
}

} // namespace

void registerRgbCameraFactory(const std::string& manufacturer_prefix, RgbCameraFactoryFn creator)
{
    if (!creator || manufacturer_prefix.empty()) {
        return;
    }

    auto& registry = getRegistry();
    std::lock_guard<std::mutex> lock(registry.mutex);

    const std::string key = toLowerCopy(manufacturer_prefix);
    auto it = std::find_if(registry.factories.begin(), registry.factories.end(), [&](const auto& entry) {
        return entry.first == key;
    });

    if (it != registry.factories.end()) {
        it->second = std::move(creator);
    } else {
        registry.factories.emplace_back(key, std::move(creator));
    }
}

void registerRgbEnumerator(RgbEnumeratorFn enumerator)
{
    if (!enumerator) return;
    auto& registry = getRegistry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    registry.enumerators.push_back(std::move(enumerator));
}

std::shared_ptr<IRgbCamera> createRgbCamera(const RgbCameraInfo& info)
{
    const std::string manufacturer(info.manufacturer);
    if (auto factory = findFactory(manufacturer)) {
        return factory();
    }

    LOG_WARN("No RGB camera factory registered for manufacturer '%s'", manufacturer.c_str());
    return nullptr;
}

std::shared_ptr<IRgbCamera> createRgbCameraBySerial(const std::string& serial_number)
{
    auto cameras = enumerateAllRgbCameras();
    auto it = std::find_if(cameras.begin(), cameras.end(), [&](const RgbCameraInfo& info) {
        return serial_number == std::string(info.serial_number);
    });

    if (it == cameras.end()) {
        LOG_WARN("RGB camera with serial '%s' not found during enumeration", serial_number.c_str());
        return nullptr;
    }

    return createRgbCamera(*it);
}

std::vector<RgbCameraInfo> enumerateAllRgbCameras()
{
    std::vector<RgbCameraInfo> all;
    for (const auto& enumerator : getEnumerators()) {
        auto cams = enumerator ? enumerator() : std::vector<RgbCameraInfo>{};
        all.insert(all.end(), cams.begin(), cams.end());
    }
    return all;
}

} // namespace evrgb
