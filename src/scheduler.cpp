#include "scheduler.hpp"

namespace duck {

exec::static_thread_pool::scheduler Scheduler::io_scheduler() {
  return io_pool_.get_scheduler();
}

exec::static_thread_pool::scheduler Scheduler::cpu_scheduler() {
  return cpu_pool_.get_scheduler();
}

} // namespace duck
