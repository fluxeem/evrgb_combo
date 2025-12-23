#include "recording/recorded_sync_reader.h"

#include <fstream>
#include <sstream>
#include "utils/evrgb_logger.h"

namespace evrgb {

RecordedSyncReader::RecordedSyncReader(Paths paths)
    : paths_(std::move(paths))
{
}

bool RecordedSyncReader::open()
{
    if (!loadCsv()) {
        return false;
    }

    if (!openVideo()) {
        return false;
    }

    if (!openEvents()) {
        return false;
    }

    cursor_ = 0;
    opened_ = true;
    return true;
}

void RecordedSyncReader::reset()
{
    if (!opened_) {
        return;
    }

    cursor_ = 0;
    cap_.set(cv::CAP_PROP_POS_FRAMES, 0);

    if (dvs_reader_) {
        dvsense::TimeStamp start_ts;
        if (dvs_reader_->getStartTimeStamp(start_ts)) {
            dvs_reader_->seekTime(start_ts);
        }
    }
}

bool RecordedSyncReader::next(Sample& out)
{
    if (!opened_ || cursor_ >= frames_.size()) {
        return false;
    }

    const FrameMeta& meta = frames_[cursor_];

    if (!cap_.read(out.rgb)) {
        return false;
    }

    out.frame_index = meta.frame_index;
    out.exposure_start_us = meta.start_us;
    out.exposure_end_us = meta.end_us;

    out.events.reset();

    if (dvs_reader_) {
        auto current_ts = dvs_reader_->getCurrentPosTimeStamp();
        const uint64_t window = (meta.end_us > current_ts) ? (meta.end_us - current_ts) : 0;
        out.events = dvs_reader_->getNTimeEvents(window);
    }
    ++cursor_;
    return true;
}

std::optional<dvsense::TimeStamp> RecordedSyncReader::getRecordingStartTimeUs() const
{
    if (dvs_reader_) {
        dvsense::TimeStamp start_ts;
        if (dvs_reader_->getStartTimeStamp(start_ts)) {
            return start_ts;
        } else {
            LOG_ERROR("RecordedSyncReader: Failed to get DVS recording start timestamp.");
            return std::nullopt;
        }
    } else {
        LOG_ERROR("RecordedSyncReader: DVS reader not initialized.");
        return std::nullopt;
    }
}

size_t RecordedSyncReader::frameCount() const
{
    return frames_.size();
}

bool RecordedSyncReader::isOpen() const
{
    return opened_;
}

std::filesystem::path RecordedSyncReader::resolve(const std::string& name) const
{
    return std::filesystem::path(paths_.recording_dir) / name;
}

bool RecordedSyncReader::loadCsv()
{
    const std::filesystem::path csv_path = resolve(paths_.csv);
    std::ifstream csv(csv_path);
    if (!csv.is_open()) {
        return false;
    }

    std::string line;
    if (!std::getline(csv, line)) {
        return false;
    }

    frames_.clear();

    while (std::getline(csv, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string f_idx, start, end;

        if (!std::getline(ss, f_idx, ',')) {
            continue;
        }
        if (!std::getline(ss, start, ',')) {
            continue;
        }
        if (!std::getline(ss, end, ',')) {
            continue;
        }

        FrameMeta meta{};
        try {
            meta.frame_index = std::stoi(f_idx);
            meta.start_us = std::stoull(start);
            meta.end_us = std::stoull(end);
        } catch (...) {
            continue;
        }

        frames_.push_back(meta);
    }

    return !frames_.empty();
}

bool RecordedSyncReader::openVideo()
{
    const std::filesystem::path video_path = resolve(paths_.video);
    cap_.open(video_path.string());
    return cap_.isOpened();
}

bool RecordedSyncReader::openEvents()
{
    const std::filesystem::path events_path = resolve(paths_.events);
    dvs_reader_ = dvsense::DvsFileReader::createFileReader(events_path.string());
    if (!dvs_reader_) {
        return false;
    }

    if (!dvs_reader_->loadFile()) {
        dvs_reader_.reset();
        return false;
    }

    dvsense::TimeStamp start_ts;
    if (!dvs_reader_->getStartTimeStamp(start_ts)) {
        dvs_reader_.reset();
        return false;
    }

    dvs_reader_->seekTime(start_ts);

    return true;
}

}  // namespace evrgb
