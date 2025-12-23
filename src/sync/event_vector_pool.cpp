#include "sync/event_vector_pool.h"

namespace evrgb {

EventVectorPool::EventVectorPool(size_t preallocated_count, size_t vector_capacity)
    : vector_capacity_(vector_capacity)
{
    pool_.reserve(preallocated_count);
    for (size_t i = 0; i < preallocated_count; ++i) {
        pool_.push_back(createVector());
    }
}

std::shared_ptr<std::vector<dvsense::Event2D>> EventVectorPool::acquire()
{
    std::shared_ptr<std::vector<dvsense::Event2D>> result;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pool_.empty()) {
            result = pool_.back();
            pool_.pop_back();
        }
    }

    if (!result) {
        result = createVector();
    }

    return result;
}

void EventVectorPool::release(std::shared_ptr<std::vector<dvsense::Event2D>> vec)
{
    if (!vec) {
        return;
    }

    vec->clear();
    if (vector_capacity_ > 0 && vec->capacity() < vector_capacity_) {
        vec->reserve(vector_capacity_);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push_back(std::move(vec));
}

std::shared_ptr<std::vector<dvsense::Event2D>> EventVectorPool::createVector() const
{
    auto vec = std::make_shared<std::vector<dvsense::Event2D>>();
    if (vector_capacity_ > 0) {
        vec->reserve(vector_capacity_);
    }
    return vec;
}

}  // namespace evrgb
