#include "scheduler.hpp"

namespace duck {

constexpr size_t io_threads = 1;
constexpr size_t cpu_threads = 1;
constexpr size_t priority_threads = 1;

Scheduler::Scheduler()
    : io_pool_{io_threads}, cpu_pool_{cpu_threads},
      priority_pool_{priority_threads} {}

Scheduler &Scheduler::instance() {
  static Scheduler instance;
  return instance;
}

exec::static_thread_pool::scheduler Scheduler::io_scheduler() {
  return instance().io_pool_.get_scheduler();
}

exec::static_thread_pool::scheduler Scheduler::cpu_scheduler() {
  return instance().cpu_pool_.get_scheduler();
}

} // namespace duck
