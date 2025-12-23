#include "sync/trigger_buffer.h"
#include "utils/evrgb_logger.h"

namespace evrgb {

TriggerBuffer::TriggerBuffer(size_t max_buffer_size)
    : max_buffer_size_(max_buffer_size)
{
}

bool TriggerBuffer::addTrigger(const TriggerSignal& trigger)
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    if (temp_trigger_pair_.empty()) {
        if (trigger.polarity == 0) {
            temp_trigger_pair_.start_trigger = trigger;
            return false;
        }

        LOG_WARN("Received end trigger before start trigger");
        trigger_pair_queue_.push(TriggerPair(std::nullopt, trigger));
        return true;
    }

    if (trigger_pair_queue_.size() >= max_buffer_size_) {
        LOG_WARN("Trigger buffer is full, ignoring new trigger");
        return false;
    }

    if (trigger.polarity == 1) {
        temp_trigger_pair_.end_trigger = trigger;
        trigger_pair_queue_.push(TriggerPair{temp_trigger_pair_.start_trigger, temp_trigger_pair_.end_trigger});
        temp_trigger_pair_.reset();
        return true;
    }

    LOG_WARN("Received start trigger while another start trigger is pending");
    trigger_pair_queue_.push(TriggerPair{temp_trigger_pair_.start_trigger, temp_trigger_pair_.end_trigger});
    temp_trigger_pair_.start_trigger = trigger;
    return true;
}

bool TriggerBuffer::getOldestTrigger(TriggerPair& trigger)
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    if (trigger_pair_queue_.empty()) {
        return false;
    }

    trigger = trigger_pair_queue_.front();
    trigger_pair_queue_.pop();
    return true;
}

bool TriggerBuffer::peekOldestTrigger(TriggerPair& trigger)
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    if (trigger_pair_queue_.empty()) {
        return false;
    }

    trigger = trigger_pair_queue_.front();
    return true;
}

bool TriggerBuffer::pop()
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);

    if (trigger_pair_queue_.empty()) {
        return false;
    }

    trigger_pair_queue_.pop();
    return true;
}

void TriggerBuffer::clear()
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    while (!trigger_pair_queue_.empty()) {
        trigger_pair_queue_.pop();
    }
}

size_t TriggerBuffer::size() const
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return trigger_pair_queue_.size();
}

bool TriggerBuffer::empty() const
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return trigger_pair_queue_.empty();
}

size_t TriggerBuffer::getMaxSize() const
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return max_buffer_size_;
}

void TriggerBuffer::setMaxSize(size_t max_size)
{
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    max_buffer_size_ = max_size;

    while (trigger_pair_queue_.size() > max_buffer_size_) {
        trigger_pair_queue_.pop();
    }
}

}  // namespace evrgb
