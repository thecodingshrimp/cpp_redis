#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

using CPPRedisValue =
    std::variant<std::string, std::vector<std::string>,
                 std::unordered_map<std::string, std::string>>;

class Storage {
public:
  Storage() = default;

  void hset(const std::string &key, const std::string &field,
            const std::string &value);
  void set(const std::string &key, const std::string &value);

  std::optional<std::string> hget(const std::string &key,
                                  const std::string &field);
  std::optional<std::string> get(const std::string &key);

  bool hdel(const std::string &key, const std::string &field);
  bool del(const std::string &key);

  void lock_mutex();
  void unlock_mutex();

  using KVPairVisitor =
      std::function<void(const std::string &, const CPPRedisValue &)>;
  void visitAll(const KVPairVisitor &visitor);

  bool
  setKVStore(const std::unordered_map<std::string, CPPRedisValue> &kv_store);

  template <typename T> std::optional<T> assertType(const std::string &key);

private:
  std::unordered_map<std::string, CPPRedisValue> kvstore_;
  std::mutex mutex_;
};

#endif
