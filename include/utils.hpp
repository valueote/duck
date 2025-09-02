#pragma once
#include <algorithm>
#include <filesystem>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace duck {

namespace fs = std::filesystem;

template <typename Key, typename Value> class Lru {
private:
  size_t capacity_;
  std::list<Key> lru_list_;
  std::unordered_map<Key, typename std::list<Key>::iterator> map_;
  std::unordered_map<Key, Value> cache_;
  std::shared_mutex lru_mutex_;
  void touch_without_lock(const Key &path) {
    lru_list_.splice(lru_list_.begin(), lru_list_, map_[path]);
  }

public:
  Lru(size_t capacity) : capacity_{capacity} {}

  std::optional<Value> get(const Key &path) {
    std::unique_lock lock{lru_mutex_};
    auto iter = map_.find(path);
    if (iter == map_.end()) {
      return std::nullopt;
    }

    touch_without_lock(path);

    return cache_[path];
  }

  void insert(const Key &path, const Value &data) {
    auto iter = map_.find(path);
    std::unique_lock lock{lru_mutex_};
    if (iter != map_.end()) {
      cache_[path] = data;
      touch_without_lock(path);
    } else {
      if (lru_list_.size() == capacity_) {
        const fs::path &lru_path = lru_list_.back();

        map_.erase(lru_path);
        cache_.erase(lru_path);

        lru_list_.pop_back();
      }

      lru_list_.push_front(path);
      map_[path] = lru_list_.begin();
      cache_[path] = data;
    }
  }
};

struct Direcotry {
  fs::path path_;
  std::vector<fs::directory_entry> entries_;
  std::vector<fs::directory_entry> hidden_entries_;
  bool empty() const { return entries_.empty() && hidden_entries_.empty(); }
};

inline std::string entry_name_with_icon(const fs::directory_entry &entry) {
  if (entry.path().empty()) {
    return "[Invalid Entry]";
  }

  static const std::unordered_map<std::string, std::string> extension_icons{
      {".txt", "\uf15c"}, {".md", "\ueeab"},   {".cpp", "\ue61d"},
      {".hpp", "\uf0fd"}, {".h", "\uf0fd"},    {".c", "\ue61e"},
      {".jpg", "\uf4e5"}, {".jpeg", "\uf4e5"}, {".png", "\uf4e5"},
      {".gif", "\ue60d"}, {".pdf", "\ue67d"},  {".zip", "\ue6aa"},
      {".mp3", "\uf001"}, {".mp4", "\uf03d"},  {".json", "\ue60b"},
      {".log", "\uf4ed"}, {".csv", "\ueefc"},
  };

  if (!fs::exists(entry) || entry.path().empty()) {
    return "";
  }

  const auto filename = entry.path().filename().string();
  if (fs::is_directory(entry)) {
    return std::format("\uf4d3 {}", filename);
  }

  auto ext = entry.path().extension().string();
  std::ranges::transform(ext, ext.begin(), [](char character) {
    return static_cast<char>(std::tolower(character));
  });

  auto icon_it = extension_icons.find(ext);
  const std::string &icon =
      icon_it != extension_icons.end() ? icon_it->second : "\uf15c";

  return std::format("{} {}", icon, filename);
}

} // namespace duck
