#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>

#include "snapshotter.hpp"
#include "storage.hpp"

Snapshotter::Snapshotter(std::shared_ptr<Storage> storage)
    : storage_(storage) {}

bool Snapshotter::save(const std::string &filename, SnapshotFormat &format) {
  if (format != SnapshotFormat::CUSTOM) {
    return false;
  }

  storage_->lock_mutex();

  auto pid = fork();
  if (pid < 0) {
    std::cerr << "ERROR: forking did not work" << std::endl;
    return false;
  } else if (pid == 0) {
    std::cout << "CHILD_PROCESS: starting..." << std::endl;
    std::ofstream outfile(filename, std::ofstream::binary);
    std::cout << filename << std::endl;
    if (!outfile.is_open()) {
      std::cerr << "Failed to open file for writing: " << filename << std::endl;
      _exit(EXIT_FAILURE);
    }

    auto writer = [&](const std::string &key, const CPPRedisValue &value) {
      if (!outfile) {
        std::cout << "CHILD_PROCESS: ERROR: file not open" << std::endl;
        _exit(EXIT_FAILURE);
      }

      // TODO adjust writer so that hashmaps are supported - maybe extend parser
      // for parsing hmap to file.
      uint32_t key_len = key.length();
      uint32_t value_len = value.length();
      outfile.write(reinterpret_cast<const char *>(&key_len), sizeof(uint32_t));
      outfile.write(key.c_str(), key.length());
      outfile.write(reinterpret_cast<const char *>(&value_len),
                    sizeof(uint32_t));
      outfile.write(value.c_str(), value.length());
    };
    storage_->visitAll(writer);
    outfile.close();
    _exit(EXIT_SUCCESS);
  }

  storage_->unlock_mutex();

  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status)) {
    std::cout << "child process exited normally." << std::endl;
    return true;
  }
  std::cout << "something went wrong" << std::endl;
  return false;
}

bool Snapshotter::load(const std::string &filename, SnapshotFormat format) {
  if (format != SnapshotFormat::CUSTOM) {
    return false;
  }

  std::ifstream infile(filename, std::ifstream::binary);

  if (!infile.is_open()) {
    std::cerr << "Failed to open file for writing: " << filename << std::endl;
    return false;
  }

  std::unordered_map<std::string, std::string> new_kvstore;
  uint32_t str_len;
  while (infile.read(reinterpret_cast<char *>(&str_len), sizeof(uint32_t))) {
    std::string str(str_len, '\0');
    if (infile.read(&str[0], str_len)) {
      if (infile.read(reinterpret_cast<char *>(&str_len), sizeof(uint32_t))) {
        std::string value(str_len, '\0');
        new_kvstore.emplace(std::make_pair(str, value));
      } else {
        std::cerr << "Error: Unexpected eof or read error." << std::endl;
        break;
      }
    } else {
      std::cerr << "Error: Unexpected eof or read error." << std::endl;
      break;
    }
  }

  return storage_->setKVStore(new_kvstore);
}
