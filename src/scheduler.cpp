#include "scheduler.h"

namespace duck {

constexpr size_t io_threads = 4;
constexpr size_t cpu_threads = 5;
constexpr size_t priority_threads = 5;

Scheduler::Scheduler()
    : io_pool_{io_threads}, cpu_pool_{cpu_threads},
      priority_pool_{priority_threads} {}

Scheduler &Scheduler::get_instance() {
  static Scheduler instance;
  return instance;
}

stdexec::scheduler auto Scheduler::io_scheduler() {
  return get_instance().io_pool_.get_scheduler();
}

stdexec::scheduler auto Scheduler::cpu_scheduler() {
  return get_instance().cpu_pool_.get_scheduler();
}

} // namespace duck
