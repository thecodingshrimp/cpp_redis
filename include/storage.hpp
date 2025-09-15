#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <cstdint>
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

  uint32_t size();
  void hset(const std::string &key, const std::string &field,
            const std::string &value);
  void ladd(const std::string &key, const std::string &value);
  void set(const std::string &key, const std::string &value);

  std::optional<std::string> hget(const std::string &key,
                                  const std::string &field);
  std::optional<std::string> lget(const std::string &key, const int &idx);
  std::optional<std::string> get(const std::string &key);

  bool hdel(const std::string &key, const std::string &field);
  bool ldel(const std::string &key, const int &idx);
  bool del(const std::string &key);

  void lock_mutex();
  void unlock_mutex();

  using KVPairVisitor =
      std::function<void(const std::string &, const CPPRedisValue &)>;
  void visitAll(const KVPairVisitor &visitor);

  bool
  setKVStore(const std::unordered_map<std::string, CPPRedisValue> &kv_store);

private:
  std::unordered_map<std::string, CPPRedisValue> kvstore_;
  std::mutex mutex_;

  template <typename T> T *get_if_type(const std::string &key);
};

#endif
