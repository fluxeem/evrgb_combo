#ifndef EVRGB_EVENT_VECTOR_POOL_H_
#define EVRGB_EVENT_VECTOR_POOL_H_

#include <memory>
#include <mutex>
#include <vector>
#include <cstdint>

#include "DvsenseBase/EventBase/EventTypes.hpp"

namespace evrgb {

class EventVectorPool {
public:
    EventVectorPool(size_t preallocated_count = 0, size_t vector_capacity = 0);

    std::shared_ptr<std::vector<dvsense::Event2D>> acquire();
    void release(std::shared_ptr<std::vector<dvsense::Event2D>> vec);

private:
    std::shared_ptr<std::vector<dvsense::Event2D>> createVector() const;

    std::vector<std::shared_ptr<std::vector<dvsense::Event2D>>> pool_;
    mutable std::mutex mutex_;
    size_t vector_capacity_ = 0;
};

}  // namespace evrgb

#endif  // EVRGB_EVENT_VECTOR_POOL_H_
