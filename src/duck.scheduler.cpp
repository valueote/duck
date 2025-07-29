module;
#include <exec/static_thread_pool.hpp>
module duck.scheduler;

namespace duck {

constexpr size_t io_threads = 4;
constexpr size_t cpu_threads = 5;
constexpr size_t priority_threads = 5;

Scheduler::Scheduler()
    : ui_pool_{1}, io_pool_{io_threads}, cpu_pool_{cpu_threads},
      priority_pool_{priority_threads} {}

Scheduler &Scheduler::get_instance() {
  static Scheduler instance;
  return instance;
}

exec::static_thread_pool::scheduler Scheduler::io_scheduler() {
  return get_instance().io_pool_.get_scheduler();
}

exec::static_thread_pool::scheduler Scheduler::cpu_scheduler() {
  return get_instance().cpu_pool_.get_scheduler();
}

exec::static_thread_pool::scheduler Scheduler::ui_scheduler() {
  return get_instance().ui_pool_.get_scheduler();
}

} // namespace duck
