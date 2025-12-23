#ifndef COMBO_THREADS_H
#define COMBO_THREADS_H

#include "core/combo_types.h"
#include "sync/trigger_buffer.h"
#include "camera/rgb_camera.h"
#include "camera/dvs_camera.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>

namespace evrgb {

class ComboThreads {
public:
    explicit ComboThreads(TriggerBuffer& trigger_buffer);
    ~ComboThreads();

    bool startThreads(std::shared_ptr<IRgbCamera> rgb_camera, std::shared_ptr<DvsCamera> dvs_camera);
    void stopThreads();
    
    void setRgbImageCallback(std::function<void(const RgbImageWithTimestamp&)> callback);
    void setDvsTriggerCallback(std::function<void(uint64_t)> callback);

    bool isRgbThreadRunning() const { return rgb_thread_running_.load(); }
    bool isDvsThreadRunning() const { return dvs_thread_running_.load(); }
    bool isSyncThreadRunning() const { return sync_thread_running_.load(); }

private:
    void rgbCaptureThread();
    void dvsTriggerThread();
    void syncThread();

    TriggerBuffer& trigger_buffer_;
    
    std::shared_ptr<IRgbCamera> rgb_camera_;
    std::shared_ptr<DvsCamera> dvs_camera_;
    
    std::thread rgb_thread_;
    std::thread dvs_thread_;
    std::thread sync_thread_;
    
    std::atomic<bool> rgb_thread_running_{false};
    std::atomic<bool> dvs_thread_running_{false};
    std::atomic<bool> sync_thread_running_{false};
    
    std::function<void(const RgbImageWithTimestamp&)> rgb_callback_;
    std::function<void(uint64_t)> dvs_callback_;
    
    std::mutex rgb_callback_mutex_;
    std::mutex dvs_callback_mutex_;
    
    // 用于停止线程的条件变量
    std::mutex stop_mutex_;
    std::condition_variable stop_cv_;
    std::atomic<bool> should_stop_{false};
};

} // namespace evrgb

#endif // COMBO_THREADS_H