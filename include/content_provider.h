#pragma once
#include "colorscheme.h"
#include "file_manager.h"
#include "scheduler.h"
#include "ui.h"
#include <filesystem>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <functional>
#include <stdexec/execution.hpp>
#include <string>

namespace duck {
class ContentProvider {
private:
  Ui &ui_;
  const ColorScheme &color_scheme_;
  std::mutex mutex_;

  std::string get_text_preview(const fs::path &path, size_t max_lines = 100,
                               size_t max_width = 80);

  std::string bat_text_preview(const std::optional<fs::path> &path,
                               size_t max_lines = 100, size_t max_width = 80);
  ftxui::Element get_directory_preview(const fs::path &dir_path);
  ftxui::Element deleted_entries();

public:
  ContentProvider(Ui &ui, const ColorScheme &color_scheme);

  std::function<ftxui::Element(const ftxui::EntryState &state)>
  entries_transform();

  ftxui::Component deletion_dialog();

  stdexec::sender auto get_directory_preview_async(const fs::path &dir_path) {
    return stdexec::on(
        Scheduler::io_scheduler(),
        stdexec::just(dir_path) | stdexec::then([this](fs::path dir_path) {
          return FileManager::get_directory_preview(ui_.selected(),
                                                    std::move(dir_path));
        }) | stdexec::then([this](ftxui::Element preview) {
          ui_.post_task([this, preview]() {
            ui_.update_entries_preview(std::move(preview));
          });
        }));

    ;
  }

  stdexec::sender auto get_text_preview_async(const fs::path &dir_path) {
    return stdexec::on(
        Scheduler::io_scheduler(),
        stdexec::just(dir_path) | stdexec::then([this](fs::path dir_path) {
          return get_text_preview(dir_path);
        }) | stdexec::then([this](std::string preview) {
          ui_.post_task([this, preview]() {
            ui_.update_text_preview(std::move(preview));
          });
        }));
  }

  std::function<ftxui::Element()> preview_async();
};
} // namespace duck
