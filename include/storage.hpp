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

  void set(const std::string& key, const std::string& value);

  std::optional<std::string> get(const std::string& key);

  bool del(const std::string& key);

  using KVPairVisitor =
      std::function<void(const std::string&, const std::string&)>;
  void visitAll(const KVPairVisitor& visitor) const;

  void setPairs(const std::vector<std::string>& raw_kvpairs);

 private:
  std::unordered_map<std::string, std::string> kvstore_;
  std::mutex mutex_;
};

#endif
