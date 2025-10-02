#include "doctest.h"
#include "utils.hpp"

TEST_CASE("LRU Cache") {
  duck::Lru<int, std::string> cache(2);

  SUBCASE("Insertion and Retrieval") {
    cache.insert(1, "one");
    cache.insert(2, "two");
    CHECK(cache.get(1) == "one");
    CHECK(cache.get(2) == "two");
  }

  SUBCASE("Eviction") {
    cache.insert(1, "one");
    cache.insert(2, "two");
    cache.insert(3, "three");
    CHECK_FALSE(cache.get(1).has_value());
    CHECK(cache.get(2) == "two");
    CHECK(cache.get(3) == "three");
  }

  SUBCASE("Update") {
    cache.insert(1, "one");
    cache.insert(2, "two");
    cache.insert(1, "new_one");
    CHECK(cache.get(1) == "new_one");
  }
}

TEST_CASE("Entries Sorter") {
  auto dir = std::filesystem::directory_entry("a_dir");
  auto file1 = std::filesystem::directory_entry("b_file");
  auto file2 = std::filesystem::directory_entry("c_file");

  SUBCASE("Directories first") {
    CHECK(duck::entries_sorter(dir, file1) == true);
    CHECK(duck::entries_sorter(file1, dir) == false);
  }

  SUBCASE("Alphabetical order for files") {
    CHECK(duck::entries_sorter(file1, file2) == true);
    CHECK(duck::entries_sorter(file2, file1) == false);
  }
}
