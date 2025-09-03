#include "storage.hpp"

void Storage::set(const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  kvstore_[key] = value;
}

std::optional<std::string> Storage::get(const std::string& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = kvstore_.find(key);
  if (it != kvstore_.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool Storage::del(const std::string& key) {
  return kvstore_.erase(key);
}