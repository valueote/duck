#include "contentprovider.h"
#include "colorscheme.h"
#include "ui.h"
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <print>

namespace duck {

ContentProvider::ContentProvider(FileManager &file_manager, Ui &ui,
                                 const ColorScheme &color_scheme)
    : file_manager_{file_manager}, ui_{ui}, color_scheme_{color_scheme} {}

std::function<ftxui::Element()> ContentProvider::preview() {
  return [this]() {
    auto left_pane =
        window(ftxui::text(" " + file_manager_.current_path().string() + " ") |
                   ftxui::bold,
               ui_.menu()->Render() | ftxui::vscroll_indicator | ftxui::frame |
                   ftxui::flex) |
        ftxui::flex;

    auto right_pane =
        window(ftxui::text(" Coneten Preview ") | ftxui::bold,
               [this] {
                 const auto selected_path_opt =
                     file_manager_.get_selected_entry(ui_.selected());
                 if (!selected_path_opt.has_value()) {
                   return ftxui::text("No item selected");
                 }
                 if (fs::is_directory(selected_path_opt.value())) {
                   return get_directory_preview(selected_path_opt) |
                          ftxui::color(color_scheme_.text());
                 }
                 return ftxui::paragraph(get_text_preview(selected_path_opt));
               }() |
                   ftxui::vscroll_indicator | ftxui::frame | ftxui::flex) |
        ftxui::flex;

    return hbox(left_pane, ftxui::separator(), right_pane);
  };
}

std::function<ftxui::Element()> ContentProvider::deleted_entries() {
  return [this]() {
    if (!file_manager_.marked_entries().empty()) {
      std::vector<ftxui::Element> lines =
          file_manager_.marked_entries() |
          std::views::transform([this](const fs::directory_entry &entry) {
            return ftxui::text(file_manager_.entry_name_with_icon(entry));
          }) |
          std::ranges::to<std::vector>();
      return ftxui::vbox({lines});
    } else {
      auto selected_path = file_manager_.get_selected_entry(ui_.selected());
      if (!selected_path.has_value()) {
        return ftxui::text("[ERROR] No file selected for deletion.");
      }
      return ftxui::vbox({
          ftxui::text(std::format("{}", selected_path->path().string())),
      });
    }
  };
}

std::string
ContentProvider::get_text_preview(const std::optional<fs::path> &path,
                                  size_t max_lines, size_t max_width) {
  if (!path.has_value()) {
    return "No file to preview";
  }

  if (fs::is_directory(path.value()))
    return "[ERROR]: Call get_text_preview on the directory";

  std::ifstream file(path.value());

  std::ostringstream oss;
  std::string line;
  size_t lines = 0;

  while (std::getline(file, line) && lines < max_lines) {

    if (line.length() > max_width) {
      line = line.substr(0, max_width - 3) + "...";
    }

    for (auto &c : line) {
      if (iscntrl(static_cast<unsigned char>(c))) {
        c = '?';
      }
    }

    oss << line << '\n';
    ++lines;
  }
  return oss.str();
}

std::string
ContentProvider::bat_text_preview(const std::optional<fs::path> &path,
                                  size_t max_lines, size_t max_width) {
  if (!path.has_value()) {
    return "No file to preview";
  }

  if (fs::is_directory(path.value())) {
    return "[ERROR]: Call get_text_preview on the directory";
  }

  try {
    namespace bp2 = boost::process::v2;
    boost::asio::io_context ctx;
    boost::asio::readable_pipe rp(ctx);

    std::string command_str =
        std::format("bat --color=always --style=plain --paging=never "
                    "--line-range=:{:d} \"{}\"",
                    max_lines, path.value().string());

    bp2::shell cmd(command_str);

    // --- 这是修改的核心部分 ---
    bp2::process proc(ctx.get_executor(), cmd.exe(), cmd.args(),
                      // 第一个初始化器：重定向 IO
                      bp2::process_stdio{{}, rp, {}},
                      // 第二个初始化器：使用正确的函数和类名来继承环境
                      bp2::process_environment(bp2::environment::current()));
    // --- 修改结束 ---

    std::string output;
    boost::system::error_code ec;
    boost::asio::read(rp, boost::asio::dynamic_buffer(output), ec);

    if (ec && ec != boost::asio::error::eof) {
      throw boost::system::system_error(ec);
    }

    proc.wait();
    return output;

  } catch (const std::exception &e) {
    // ... Fallback 代码保持不变 ...
    std::println(stderr, "[WARN] Failed to use 'bat' with Boost.Process : {}",
                 e.what());
    // ...
    return "[INFO] For syntax highlighting, please install 'bat'.";
  }
}

ftxui::Element ContentProvider::get_directory_preview(
    const std::optional<fs::path> &dir_path) {
  if (!dir_path.has_value()) {
    return ftxui::text("Nothing to preview");
  }
  if (!fs::is_directory(dir_path.value())) {
    return ftxui::text("[ERROR]: Call get_directory_preview on the file");
  }

  file_manager_.update_preview_entries(ui_.selected());

  std::vector<ftxui::Element> lines =
      file_manager_.preview_entries() |
      std::views::transform([this](const fs::directory_entry &entry) {
        return ftxui::text(file_manager_.format_directory_entries(entry));
      }) |
      std::ranges::to<std::vector>();

  if (lines.empty()) {
    lines.push_back(ftxui::text("[Empty folder]"));
  }

  return ftxui::vbox(std::move(lines));
}

} // namespace duck
