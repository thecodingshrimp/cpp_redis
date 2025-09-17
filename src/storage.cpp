#include "storage.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

template <typename T> T *Storage::get_if_type(const std::string &key) {
  auto it = kvstore_.find(key);
  if (it != kvstore_.end()) {
    return std::get_if<T>(&it->second);
  }
  return nullptr;
}

uint32_t Storage::size() { return kvstore_.size(); }

void Storage::hset(const std::string &key, const std::string &field,
                   const std::string &value) {
  if (auto *map =
          get_if_type<std::unordered_map<std::string, std::string>>(key)) {
    (*map)[field] = value;
    return;
  }
  kvstore_[key] = std::unordered_map<std::string, std::string>{{field, value}};
}

void Storage::ladd(const std::string &key, const std::string &value) {
  if (auto *list = get_if_type<std::vector<std::string>>(key)) {
    list->push_back(std::move(value));
    return;
  }
  kvstore_[key] = std::vector<std::string>{value};
}

void Storage::set(const std::string &key, const std::string &value) {
  if (get_if_type<std::string>(key) || kvstore_.find(key) == kvstore_.end()) {
    kvstore_[key] = value;
  }
}

std::optional<std::string> Storage::hget(const std::string &key,
                                         const std::string &field) {
  if (auto *map =
          get_if_type<std::unordered_map<std::string, std::string>>(key)) {
    return (*map)[field];
  };

  return std::nullopt;
}

std::optional<std::string> Storage::lget(const std::string &key,
                                         const int &idx) {
  if (auto *list = get_if_type<std::vector<std::string>>(key)) {
    if (idx >= list->size()) {
      return std::nullopt;
    }
    return (*list)[idx];
  };

  return std::nullopt;
}

std::optional<std::string> Storage::get(const std::string &key) {
  if (auto *ptr = get_if_type<std::string>(key)) {
    return *ptr;
  };

  return std::nullopt;
}

bool Storage::ldel(const std::string &key, const int &idx) {
  if (auto *list = get_if_type<std::vector<std::string>>(key)) {
    if (idx >= list->size()) {
      return false;
    }
    list->erase(list->begin() + idx);
    return true;
  };

  return false;
}

bool Storage::hdel(const std::string &key, const std::string &field) {
  if (auto map =
          get_if_type<std::unordered_map<std::string, std::string>>(key)) {
    auto field_it = map->find(field);
    if (field_it != map->end()) {
      return map->erase(field);
    }
  };

  return false;
}

bool Storage::del(const std::string &key) { return kvstore_.erase(key); }
// CAUTION: not thread safe.
void Storage::visitAll(const KVPairVisitor &visitor) {
  for (const auto &pair : kvstore_) {
    visitor(pair.first, pair.second);
  }
}

bool Storage::setKVStore(
    const std::unordered_map<std::string, CPPRedisValue> &kv_store) {
  kvstore_ = kv_store;
  return true;
}
