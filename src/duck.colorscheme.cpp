module;
#include <ftxui/screen/color.hpp>
module duck.colorscheme;

namespace duck {
ftxui::Color ColorScheme::text() const { return color_map_.at("text"); }

ftxui::Color ColorScheme::border() const { return color_map_.at("border"); }

ftxui::Color ColorScheme::surface0() const { return color_map_.at("surface0"); }

ftxui::Color ColorScheme::selected() const { return color_map_.at("selected"); }

ftxui::Color ColorScheme::warning() const { return color_map_.at("warning"); }

} // namespace duck
