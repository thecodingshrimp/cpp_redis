#include "storage.hpp"
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <unistd.h>
#include <unordered_map>

void Storage::set(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(mutex_);
  // TODO what if the key is already set?
  kvstore_[key] = value;
}

std::optional<std::string> Storage::get(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = kvstore_.find(key);
  if (it != kvstore_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void Storage::lock_mutex() { mutex_.lock(); }

void Storage::unlock_mutex() { mutex_.unlock(); }

bool Storage::del(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  return kvstore_.erase(key);
}

// CAUTION: not thread safe.
void Storage::visitAll(const KVPairVisitor &visitor) {
  std::cout << "at least here" << std::endl;
  for (const auto &pair : kvstore_) {
    visitor(pair.first, pair.second);
  }
}

bool Storage::setKVStore(
    const std::unordered_map<std::string, std::string> &kv_store) {
  std::lock_guard<std::mutex> lock(mutex_);
  kvstore_ = kv_store;
  return true;
}
