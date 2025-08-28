#include "colorscheme.h"
#include <ftxui/screen/color.hpp>
namespace duck {
ftxui::Color ColorScheme::text() { return color_map_.at("text"); }

ftxui::Color ColorScheme::border() { return color_map_.at("border"); }

ftxui::Color ColorScheme::surface0() { return color_map_.at("surface0"); }

ftxui::Color ColorScheme::selected() { return color_map_.at("selected"); }

ftxui::Color ColorScheme::warning() { return color_map_.at("warning"); }

} // namespace duck
