#pragma once

#include <exec/static_thread_pool.hpp>
#include <stdexec/concepts.hpp>
#include <stdexec/execution.hpp>

namespace duck {

class Scheduler {
private:
  exec::static_thread_pool ui_pool_;
  exec::static_thread_pool io_pool_;
  exec::static_thread_pool cpu_pool_;
  exec::static_thread_pool priority_pool_;

  Scheduler();

  static Scheduler &instance();

public:
  ~Scheduler() = default;
  Scheduler(const Scheduler &) = delete;
  Scheduler &operator=(const Scheduler &) = delete;
  Scheduler(Scheduler &&) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  static exec::static_thread_pool::scheduler io_scheduler();

  static exec::static_thread_pool::scheduler cpu_scheduler();

  static exec::static_thread_pool::scheduler ui_scheduler();

  static stdexec::scheduler auto priority_scheduler();
};

} // namespace duck
