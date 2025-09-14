#pragma once
#include "app_event.hpp"
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace duck {

class EventBus {
private:
  std::queue<AppEvent> event_queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  static constexpr size_t MAX_QUEUE_SIZE = 1000;

public:
  EventBus() = default;
  ~EventBus() = default;

  EventBus(const EventBus &) = delete;
  EventBus &operator=(const EventBus &) = delete;
  EventBus(EventBus &&) = delete;
  EventBus &operator=(EventBus &&) = delete;

  void push_event(AppEvent event);

  std::optional<AppEvent> pop_event();

  std::optional<AppEvent>
  pop_event_with_timeout(std::chrono::milliseconds timeout_ms);

  std::optional<AppEvent> try_pop_event();

  bool empty() const;

  size_t size() const;
};

} // namespace duck
