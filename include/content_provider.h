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
#include <vector>
namespace duck {
class ContentProvider {
private:
  Ui &ui_;
  FileManager &file_manager_;
  const ColorScheme &color_scheme_;

  std::string get_text_preview(const fs::path &path, size_t max_lines = 100,
                               size_t max_width = 80);

  std::string bat_text_preview(const std::optional<fs::path> &path,
                               size_t max_lines = 100, size_t max_width = 80);
  ftxui::Element get_directory_preview(const fs::path &dir_path);
  ftxui::Element deleted_entries();

public:
  ContentProvider(FileManager &file_manager, Ui &ui,
                  const ColorScheme &color_scheme);

  std::function<ftxui::Element(const ftxui::EntryState &state)>
  entries_transform();
  std::function<ftxui::Element()> preview();
  ftxui::Component deletion_dialog();

  stdexec::sender auto get_directory_preview_async(const fs::path &dir_path) {
    return stdexec::on(
        Scheduler::io_scheduler(),
        stdexec::just(dir_path) | stdexec::then([this](fs::path dir_path) {
          return get_directory_preview(std::move(dir_path));
        }) | stdexec::then([this](ftxui::Element preview) {
          ui_.post_task([this, preview]() {
            ui_.update_entries_preview(std::move(preview));
          });
        }));

    ;
  }

  std::function<ftxui::Element()> preview_async();
};
} // namespace duck
