#include "event_bus.hpp"
#include <chrono>

namespace duck {

void EventBus::push_event(AppEvent event) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (event_queue_.size() >= MAX_QUEUE_SIZE) {
      event_queue_.pop();
    }
    event_queue_.emplace(event);
  }
  cv_.notify_one();
}

std::optional<AppEvent> EventBus::pop_event() {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [this]() { return !event_queue_.empty(); });

  if (event_queue_.empty()) {
    return std::nullopt;
  }

  auto event_pair = event_queue_.front();
  event_queue_.pop();
  return event_pair;
}

std::optional<AppEvent>
EventBus::pop_event_with_timeout(std::chrono::milliseconds timeout_ms) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!cv_.wait_for(lock, timeout_ms,
                    [this]() { return !event_queue_.empty(); })) {
    return std::nullopt;
  }

  if (event_queue_.empty()) {
    return std::nullopt;
  }

  auto event_pair = event_queue_.front();
  event_queue_.pop();
  return event_pair;
}

std::optional<AppEvent> EventBus::try_pop_event() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (event_queue_.empty()) {
    return std::nullopt;
  }

  auto event_pair = event_queue_.front();
  event_queue_.pop();
  return event_pair;
}

bool EventBus::empty() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return event_queue_.empty();
}

size_t EventBus::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return event_queue_.size();
}

} // namespace duck
