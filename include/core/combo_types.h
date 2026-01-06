#ifndef EVRGB_COMBO_TYPES_H_
#define EVRGB_COMBO_TYPES_H_

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
    #include "opencv2/opencv.hpp"
#else
    #include "opencv4/opencv2/opencv.hpp"
#endif
#include "DvsenseBase/EventBase/EventTypes.hpp"
#include <nlohmann/json.hpp>

#include "utils/calib_info.h"

#ifdef _WIN32
    #ifdef EVRGB_EXPORTS
        #define EVRGB_API __declspec(dllexport)
    #else
        #define EVRGB_API __declspec(dllimport)
    #endif
#else
    #define EVRGB_API   
#endif

namespace evrgb {

/** Arrangement options for a combo system. */
enum class ComboArrangement {
    STEREO = 0,
    BEAM_SPLITTER = 1
};

/**
 * @brief Trigger signal structure to hold trigger event information
 */
struct EVRGB_API TriggerSignal
{
    short trigger_id = 0;
    short polarity = 0;
    uint64_t timestamp_us = 0;

    TriggerSignal() = default;

    TriggerSignal(uint64_t ts_us, short id = 0, short pol = 0)
        : trigger_id(id), polarity(pol), timestamp_us(ts_us) {}

    explicit TriggerSignal(const dvsense::EventTriggerIn& event)
        : trigger_id(event.id), polarity(event.polarity), timestamp_us(event.timestamp) {}
};

struct EVRGB_API TriggerPair
{
    std::optional<TriggerSignal> start_trigger = std::nullopt;
    std::optional<TriggerSignal> end_trigger = std::nullopt;

    TriggerPair() = default;
    TriggerPair(const TriggerSignal& start, const TriggerSignal& end)
        : start_trigger(start), end_trigger(end) {}

    explicit TriggerPair(std::optional<TriggerSignal> start, std::optional<TriggerSignal> end)
        : start_trigger(std::move(start)), end_trigger(std::move(end)) {}

    bool empty() const {
        return (!start_trigger.has_value() && !end_trigger.has_value());
    }

    void reset() {
        start_trigger = std::nullopt;
        end_trigger = std::nullopt;
    }
};

struct EVRGB_API RgbImageWithTimestamp
{
    cv::Mat image;
    uint64_t exposure_start_ts = 0;
    uint64_t exposure_end_ts = 0;
    uint32_t image_index = 0;

    RgbImageWithTimestamp() = default;

    RgbImageWithTimestamp(const cv::Mat& img, uint64_t start_ts, uint64_t end_ts, uint32_t idx = 0)
        : image(img), exposure_start_ts(start_ts), exposure_end_ts(end_ts), image_index(idx) {}
};

using RgbImageCallback = std::function<void(const cv::Mat&)>;
using SyncedCallback = std::function<void(const RgbImageWithTimestamp&, const std::vector<dvsense::Event2D>&)>;

/** Per-camera metadata used for persistence. */
struct EVRGB_API CameraMetadata {
    std::string manufacturer;
    std::string model;
    std::string serial;
    unsigned int width = 0;
    unsigned int height = 0;
    std::optional<CameraIntrinsics> intrinsics;
};

/** Aggregated combo metadata for saving/loading. */
struct EVRGB_API ComboMetadata {
    CameraMetadata rgb;
    CameraMetadata dvs;
    ComboArrangement arrangement{ComboArrangement::STEREO};
    ComboCalibrationInfo calibration{};
};

// Arrangement helpers
EVRGB_API const std::string toString(ComboArrangement arrangement);
EVRGB_API ComboArrangement arrangementFromString(const std::string& value);

// JSON converters (API names)
EVRGB_API void toJson(nlohmann::json& j, const CameraMetadata& metadata);
EVRGB_API void fromJson(const nlohmann::json& j, CameraMetadata& metadata);
EVRGB_API void toJson(nlohmann::json& j, const ComboMetadata& metadata);
EVRGB_API void fromJson(const nlohmann::json& j, ComboMetadata& metadata);

// nlohmann ADL entry points
EVRGB_API void to_json(nlohmann::json& j, const CameraMetadata& metadata);
EVRGB_API void from_json(const nlohmann::json& j, CameraMetadata& metadata);
EVRGB_API void to_json(nlohmann::json& j, const ComboMetadata& metadata);
EVRGB_API void from_json(const nlohmann::json& j, ComboMetadata& metadata);
}  // namespace evrgb

#endif  // EVRGB_COMBO_TYPES_H_
