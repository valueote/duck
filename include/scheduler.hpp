#pragma once

#include <exec/static_thread_pool.hpp>
#include <stdexec/concepts.hpp>
#include <stdexec/execution.hpp>

namespace duck {

class Scheduler {
private:
  static inline exec::static_thread_pool io_pool_{1};
  static inline exec::static_thread_pool cpu_pool_{1};
  static inline exec::static_thread_pool priority_pool_{1};

public:
  static exec::static_thread_pool::scheduler io_scheduler();

  static exec::static_thread_pool::scheduler cpu_scheduler();

  static stdexec::scheduler auto priority_scheduler();
};

} // namespace duck
