#include "recording/synced_data_recorder.h"
#include "utils/evrgb_logger.h"

#include <filesystem>
#include <utility>

namespace evrgb {

SyncedDataRecorder::SyncedDataRecorder(SyncedRecorderConfig config)
    : config_(std::move(config))
{
}

bool SyncedDataRecorder::start(const SyncedRecorderConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    started_ = false;
    frame_size_ = cv::Size(0, 0);

    if (config_.output_dir.empty()) {
        LOG_WARN("Recorder start skipped: output directory is empty");
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(config_.output_dir, ec);
    if (ec) {
        LOG_WARN("Failed to create output directory '%s' (%s)", config_.output_dir.c_str(), ec.message().c_str());
    }

    std::filesystem::path out_dir(config_.output_dir);
    rgb_path_ = (out_dir / "combo_rgb.mp4").string();
    csv_path_ = (out_dir / "combo_timestamps.csv").string();
    dvs_raw_path_ = (out_dir / "combo_events.raw").string();

    csv_stream_.open(csv_path_, std::ios::out | std::ios::trunc);
    if (!csv_stream_.is_open()) {
        LOG_ERROR("Failed to open CSV file: %s", csv_path_.c_str());
        return false;
    }
    csv_stream_ << "frame_index,exposure_start_us,exposure_end_us" << '\n';

    started_ = true;
    return true;
}

bool SyncedDataRecorder::ensureVideoWriter(const cv::Mat& frame)
{
    if (writer_.isOpened()) {
        return true;
    }

    frame_size_ = frame.size();
    if (frame_size_.width <= 0 || frame_size_.height <= 0) {
        LOG_ERROR("Invalid frame size for video writer: %dx%d", frame_size_.width, frame_size_.height);
        return false;
    }

    const double fps = (config_.fps > 0.0) ? config_.fps : 30.0;
    const int fourcc = cv::VideoWriter::fourcc(
        config_.fourcc.size() > 0 ? config_.fourcc[0] : 'm',
        config_.fourcc.size() > 1 ? config_.fourcc[1] : 'p',
        config_.fourcc.size() > 2 ? config_.fourcc[2] : '4',
        config_.fourcc.size() > 3 ? config_.fourcc[3] : 'v');

    if (!writer_.open(rgb_path_, fourcc, fps, frame_size_, true)) {
        LOG_ERROR("Failed to open VideoWriter for %s", rgb_path_.c_str());
        return false;
    }

    LOG_DEBUG("VideoWriter opened: path=%s size=%dx%d fps=%.2f fourcc=%s",
             rgb_path_.c_str(), frame_size_.width, frame_size_.height, fps, config_.fourcc.c_str());

    return true;
}

cv::Mat SyncedDataRecorder::toBgr8(const cv::Mat& input) const
{
    if (input.empty()) {
        return {};
    }

    const auto depthScale = [](int depth) {
        switch (depth) {
            case CV_16U: return 1.0 / 256.0;
            case CV_32F:
            case CV_64F: return 255.0;
            default:     return 1.0;
        }
    };

    if (input.channels() == 3) {
        if (input.type() == CV_8UC3) {
            return input;
        }
        cv::Mat converted;
        input.convertTo(converted, CV_8UC3, depthScale(input.depth()));
        return converted;
    }

    cv::Mat temp;
    input.convertTo(temp, CV_8U, depthScale(input.depth()));

    if (temp.channels() == 1) {
        cv::Mat bgr;
        cv::cvtColor(temp, bgr, cv::COLOR_GRAY2BGR);
        return bgr;
    }

    return {};
}

void SyncedDataRecorder::writeCsvRow(const RgbImageWithTimestamp& rgb)
{
    if (!csv_stream_.is_open()) {
        return;
    }

    csv_stream_ << rgb.image_index << ','
                << rgb.exposure_start_ts << ','
                << rgb.exposure_end_ts << '\n';
}

bool SyncedDataRecorder::record(const RgbImageWithTimestamp& rgb, const std::vector<dvsense::Event2D>& events)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_) {
        return false;
    }

    cv::Mat bgr = toBgr8(rgb.image);
    if (bgr.empty()) {
        LOG_WARN("Skipping frame write: empty or unsupported image");
    } else {
        if (!ensureVideoWriter(bgr)) {
            return false;
        }
        writer_.write(bgr);
    }

    writeCsvRow(rgb);
    return true;
}

void SyncedDataRecorder::stop()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (writer_.isOpened()) {
        writer_.release();
    }

    if (csv_stream_.is_open()) {
        csv_stream_.flush();
        csv_stream_.close();
    }

    started_ = false;
}

bool SyncedDataRecorder::isActive() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return started_;
}

bool SyncedDataRecorder::requiresDvsRawRecording() const
{
    return !dvs_raw_path_.empty();
}

const std::string& SyncedDataRecorder::dvsRawPath() const
{
    return dvs_raw_path_;
}

const std::string& SyncedDataRecorder::rgbPath() const
{
    return rgb_path_;
}

const std::string& SyncedDataRecorder::csvPath() const
{
    return csv_path_;
}

}  // namespace evrgb
