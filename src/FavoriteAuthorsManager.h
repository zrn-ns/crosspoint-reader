#pragma once

#include <vector>

struct FavoriteAuthor {
  int authorId;
  char name[48];
  char kana[48];
};

class FavoriteAuthorsManager {
 public:
  static constexpr const char* FAVORITES_PATH = "/Aozora/.favorite_authors.json";

  bool load();
  bool save() const;
  void addAuthor(int id, const char* name, const char* kana);
  void removeAuthor(int id);
  bool isFavorited(int id) const;
  const std::vector<FavoriteAuthor>& entries() const { return entries_; }

 private:
  std::vector<FavoriteAuthor> entries_;
  void sortEntries();
};
