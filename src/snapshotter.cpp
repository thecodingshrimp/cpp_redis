#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <variant>

#include "snapshotter.hpp"
#include "storage.hpp"

Snapshotter::Snapshotter(std::shared_ptr<Storage> storage)
    : storage_(storage) {}

bool write_uint8(std::ofstream &outfile, const uint8_t &c) {
  outfile.write(reinterpret_cast<const char *>(&c), sizeof(uint8_t));
  return outfile.good();
}

bool read_uint8(std::ifstream &infile, uint8_t &c) {
  return !!infile.read(reinterpret_cast<char *>(&c), sizeof(uint8_t));
}

bool write_uint32(std::ofstream &outfile, const uint32_t &number) {
  outfile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
  return outfile.good();
}

bool read_uint32(std::ifstream &infile, uint32_t &number) {
  return !!infile.read(reinterpret_cast<char *>(&number), sizeof(uint32_t));
}

bool write_string(std::ofstream &outfile, const std::string &str) {
  if (!write_uint32(outfile, str.length())) {
    return false;
  }
  outfile.write(str.c_str(), str.length());
  return outfile.good();
}

bool read_string(std::ifstream &infile, std::string &str) {
  uint32_t len;
  if (!read_uint32(infile, len)) {
    return false;
  }
  str.resize(len);
  return !!infile.read(&str[0], len);
}

bool Snapshotter::save(const std::string &filename, SnapshotFormat &format) {
  if (format != SnapshotFormat::CUSTOM) {
    return false;
  }

  auto pid = fork();
  if (pid < 0) {
    std::cerr << "ERROR: forking did not work" << std::endl;
    return false;
  } else if (pid == 0) {
    std::cout << "Saving kvstore state..." << std::endl;
    std::ofstream outfile(filename, std::ofstream::binary);
    if (!outfile.is_open()) {
      std::cerr << "Failed to open file for writing: " << filename << std::endl;
      _exit(EXIT_FAILURE);
    }

    auto writer = [&](const std::string &key, const CPPRedisValue &value) {
      if (!outfile) {
        std::cout << "CHILD_PROCESS: ERROR: file not open" << std::endl;
        _exit(EXIT_FAILURE);
      }

      if (const auto &hashmap =
              std::get_if<std::unordered_map<std::string, std::string>>(
                  &value)) {
        write_uint8(outfile, HMAP);
        write_string(outfile, key);
        write_uint32(outfile, hashmap->size());
        for (const auto &kv : *hashmap) {
          write_string(outfile, kv.first);
          write_string(outfile, kv.second);
        }
      } else if (const auto &str = std::get_if<std::string>(&value)) {
        write_uint8(outfile, MAP);
        write_string(outfile, key);
        write_string(outfile, *str);
      } else if (const auto vec =
                     std::get_if<std::vector<std::string>>(&value)) {
        write_uint8(outfile, LIST);
        write_string(outfile, key);
        write_uint32(outfile, vec->size());
        for (const auto &str : *vec) {
          write_string(outfile, str);
        }
      } else {
        std::cout << "ERROR CHILD_PROCESS: value type not found." << std::endl;
      }
    };
    // size of kv store
    write_uint32(outfile, storage_->size());
    storage_->visitAll(writer);
    outfile.close();
    _exit(EXIT_SUCCESS);
  }

  return true;
}

bool Snapshotter::load(const std::string &filename, SnapshotFormat &format) {
  if (format != SnapshotFormat::CUSTOM) {
    return false;
  }

  std::ifstream infile(filename, std::ifstream::binary);
  if (!infile.is_open()) {
    std::cerr << "Failed to open file for writing: " << filename << std::endl;
    return false;
  }

  std::unordered_map<std::string, CPPRedisValue> new_kvstore;
  uint32_t kvstore_size, curr_size;
  if (!read_uint32(infile, kvstore_size)) {
    std::cerr << "ERROR CHILD_PROCESS: could not read kvstore_size"
              << std::endl;
    return false;
  }
  new_kvstore.reserve(kvstore_size);
  uint8_t type;
  for (int i = 0; i < kvstore_size; ++i) {
    if (!read_uint8(infile, type)) {
      std::cerr << "ERROR CHILD_PROCESS: could not read type" << std::endl;
      return false;
    }

    std::string key;
    if (!read_string(infile, key)) {
      std::cerr << "ERROR CHILD_PROCESS: could not read key" << std::endl;
      return false;
    }

    if (type == MAP) {
      std::string value;
      if (!read_string(infile, value)) {
        std::cerr << "ERROR CHILD_PROCESS: could not read MAP value"
                  << std::endl;
        return false;
      }
      new_kvstore.emplace(std::move(key), std::move(value));
    } else if (type == HMAP) {
      if (!read_uint32(infile, curr_size)) {
        std::cerr << "ERROR CHILD_PROCESS: could not read HMAP size"
                  << std::endl;
      }
      std::unordered_map<std::string, std::string> hashmap;
      hashmap.reserve(curr_size);
      for (int i = 0; i < curr_size; ++i) {
        std::string key, value;
        if (!read_string(infile, key) || !read_string(infile, value)) {
          std::cerr << "ERROR CHILD_PROCESS: could not read kv from hashmap"
                    << std::endl;
        }
        hashmap.emplace(std::move(key), std::move(value));
      }
      new_kvstore.emplace(std::move(key), std::move(hashmap));
    } else if (type == LIST) {
      if (!read_uint32(infile, curr_size)) {
        std::cerr << "ERROR CHILD_PROCESS: could not read LIST size"
                  << std::endl;
      }
      std::vector<std::string> list;
      list.reserve(curr_size);
      for (int i = 0; i < curr_size; ++i) {
        std::string item;
        if (!read_string(infile, item)) {
          std::cerr << "ERROR CHILD_PROCESS: could not read item from list"
                    << std::endl;
        }
        list.push_back(std::move(item));
      }
      new_kvstore.emplace(std::move(key), std::move(list));
    } else {
      std::cerr << "error: unexpected token" << std::endl;
      break;
    }
  }

  return storage_->setKVStore(new_kvstore);
}
