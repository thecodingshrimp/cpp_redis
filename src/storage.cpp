#include "storage.hpp"

void Storage::set(const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  // TODO what if the key is already set?
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
  std::lock_guard<std::mutex> lock(mutex_);
  return kvstore_.erase(key);
}

void Storage::visitAll(const KVPairVisitor& visitor) const {
  for (const auto& pair : kvstore_) {
    visitor(pair.first, pair.second);
  }
}

void Storage::setPairs(const std::vector<std::string>& raw_kvpairs) {
  kvstore_.clear();

  if (raw_kvpairs.size() < 2) {
    return;
  }

  for (int i = 0; i + 1 < raw_kvpairs.size(); i += 2) {
    kvstore_.emplace(std::make_pair(raw_kvpairs[i], raw_kvpairs[i + 1]));
  }
}
