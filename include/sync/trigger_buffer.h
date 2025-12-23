#ifndef EVRGB_TRIGGER_BUFFER_H_
#define EVRGB_TRIGGER_BUFFER_H_

#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include "core/combo_types.h"

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

class EVRGB_API TriggerBuffer
{
public:
    explicit TriggerBuffer(size_t max_buffer_size = 100);

    bool addTrigger(const TriggerSignal& trigger);
    bool getOldestTrigger(TriggerPair& trigger);
    bool peekOldestTrigger(TriggerPair& trigger);
    bool pop();
    void clear();
    size_t size() const;
    bool empty() const;
    size_t getMaxSize() const;
    void setMaxSize(size_t max_size);

private:
    mutable std::mutex buffer_mutex_;
    std::queue<TriggerPair> trigger_pair_queue_;
    size_t max_buffer_size_;

    TriggerPair temp_trigger_pair_;
};

}  // namespace evrgb

#endif  // EVRGB_TRIGGER_BUFFER_H_
