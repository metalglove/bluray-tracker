#pragma once

#include "../../domain/models.hpp"
#include <optional>
#include <vector>

namespace bluray::infrastructure {

struct PriceHistoryEntry {
  int id;
  int wishlist_id;
  double price;
  bool in_stock;
  std::string recorded_at;
};

class PriceHistoryRepository {
public:
  void addEntry(int wishlist_id, double price, bool in_stock);
  std::vector<PriceHistoryEntry> getHistory(int wishlist_id, int days = 180);
  void pruneHistory(int days_to_keep = 365);
};

} // namespace bluray::infrastructure
