#pragma once
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/color_info.hpp>
#include <unordered_map>

namespace duck {

enum class Theme { CatppuccinFrappe };

class ColorScheme {
private:
  struct CatppuccinFrappe {
    ftxui::Color Rosewater = ftxui::Color::RGB(242, 213, 207); // #f2d5cf
    ftxui::Color Flamingo = ftxui::Color::RGB(238, 190, 190);  // #eebebe
    ftxui::Color Pink = ftxui::Color::RGB(244, 184, 228);      // #f4b8e4
    ftxui::Color Mauve = ftxui::Color::RGB(202, 158, 230);     // #ca9ee6
    ftxui::Color Red = ftxui::Color::RGB(231, 130, 132);       // #e78284
    ftxui::Color Maroon = ftxui::Color::RGB(234, 153, 156);    // #ea999c
    ftxui::Color Peach = ftxui::Color::RGB(239, 159, 118);     // #ef9f76
    ftxui::Color Yellow = ftxui::Color::RGB(229, 200, 144);    // #e5c890
    ftxui::Color Green = ftxui::Color::RGB(166, 209, 137);     // #a6d189
    ftxui::Color Teal = ftxui::Color::RGB(129, 200, 190);      // #81c8be
    ftxui::Color Sky = ftxui::Color::RGB(153, 209, 219);       // #99d1db
    ftxui::Color Sapphire = ftxui::Color::RGB(133, 193, 220);  // #85c1dc
    ftxui::Color Blue = ftxui::Color::RGB(140, 170, 238);      // #8caaee
    ftxui::Color Lavender = ftxui::Color::RGB(186, 187, 241);  // #babbf1
    ftxui::Color Text = ftxui::Color::RGB(198, 208, 245);      // #c6d0f5
    ftxui::Color Subtext1 = ftxui::Color::RGB(181, 191, 226);  // #b5bfe2
    ftxui::Color Subtext0 = ftxui::Color::RGB(165, 173, 206);  // #a5adce
    ftxui::Color Overlay2 = ftxui::Color::RGB(148, 156, 187);  // #949cbb
    ftxui::Color Overlay1 = ftxui::Color::RGB(131, 139, 167);  // #838ba7
    ftxui::Color Overlay0 = ftxui::Color::RGB(115, 121, 148);  // #737994
    ftxui::Color Surface2 = ftxui::Color::RGB(98, 104, 128);   // #626880
    ftxui::Color Surface1 = ftxui::Color::RGB(81, 87, 109);    // #51576d
    ftxui::Color Surface0 = ftxui::Color::RGB(65, 69, 89);     // #414559
    ftxui::Color Base = ftxui::Color::RGB(48, 52, 70);         // #303446
    ftxui::Color Mantle = ftxui::Color::RGB(41, 44, 60);       // #292c3c
    ftxui::Color Crust = ftxui::Color::RGB(35, 38, 52);        // #232634
  } CatppuccinFrappe;
  std::unordered_map<std::string, ftxui::Color> color_map_ = {
      {"text", CatppuccinFrappe.Text},
      {"border", CatppuccinFrappe.Blue},
      {"surface0", CatppuccinFrappe.Surface0},
      {"selected", CatppuccinFrappe.Teal},
      {"warning", CatppuccinFrappe.Yellow},
  };

public:
  ftxui::Color text() const;
  ftxui::Color border() const;
  ftxui::Color surface0() const;
  ftxui::Color selected() const;
  ftxui::Color warning() const;
};

} // namespace duck
