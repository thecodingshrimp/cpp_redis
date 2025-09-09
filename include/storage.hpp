#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class Storage {
public:
  Storage() = default;

  void set(const std::string &key, const std::string &value);

  std::optional<std::string> get(const std::string &key);

  bool del(const std::string &key);

  void lock_mutex();

  void unlock_mutex();

  using KVPairVisitor =
      std::function<void(const std::string &, const std::string &)>;
  void visitAll(const KVPairVisitor &visitor);

  bool setKVStore(const std::unordered_map<std::string, std::string> &kv_store);

private:
  std::unordered_map<std::string, std::string> kvstore_;
  std::mutex mutex_;
};

#endif
