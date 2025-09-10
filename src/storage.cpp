#include "storage.hpp"
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <variant>

template <typename T>
std::optional<T> Storage::assertType(const std::string &key) {
  auto it = kvstore_.find(key);
  if (it != kvstore_.end() && std::holds_alternative<T>(it->second)) {
    return std::get<T>(it->second);
  }
  return std::nullopt;
}

void Storage::hset(const std::string &key, const std::string &field,
                   const std::string &value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto map = assertType<std::unordered_map<std::string, std::string>>(key);
  if (!map) {
    // TODO should have a sofisticated error handling here
    return;
  }
  map->at(field) = value;
}

void Storage::set(const std::string &key, const std::string &value) {
  std::lock_guard<std::mutex> lock(mutex_);
  // TODO what if the key is already set?
  kvstore_[key] = value;
}

std::optional<std::string> Storage::hget(const std::string &key,
                                         const std::string &field) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto map = assertType<std::unordered_map<std::string, std::string>>(key);
  if (!map) {
    return std::nullopt;
  }
  auto it = map->find(key);
  if (it != map->end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<std::string> Storage::get(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  // TODO return whole list/hashmap
  return assertType<std::string>(key);
}

void Storage::lock_mutex() { mutex_.lock(); }

void Storage::unlock_mutex() { mutex_.unlock(); }

bool Storage::hdel(const std::string &key, const std::string &field) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto map = assertType<std::unordered_map<std::string, std::string>>(key);
  return map->erase(field);
}

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
    const std::unordered_map<std::string, CPPRedisValue> &kv_store) {
  std::lock_guard<std::mutex> lock(mutex_);
  kvstore_ = kv_store;
  return true;
}
