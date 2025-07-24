#include "colorscheme.h"
#include <ftxui/screen/color.hpp>
#include <string>
namespace duck {
ftxui::Color ColorScheme::get(const std::string &content) {
  return colormap.at(content);
}

std::unordered_map<std::string, ftxui::Color> ColorScheme::colormap = {
    {"text", ftxui::Color::RGB(198, 208, 245)},
    {"border", ftxui::Color::RGB(186, 187, 241)},
    {"surface0", ftxui::Color::RGB(65, 69, 89)},
    {"selected", ftxui::Color::RGB(140, 170, 238)}};

} // namespace duck
