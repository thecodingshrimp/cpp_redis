#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <optional>
#include <string>
#include <unordered_map>
#include <mutex>

class Storage {
public:
  Storage() = default;

  void set(const std::string& key, const std::string& value);

  std::optional<std::string> get(const std::string& key);
  
  bool del(const std::string& key);

private:
  std::unordered_map<std::string, std::string> kvstore_;
  std::mutex mutex_;
};

#endif