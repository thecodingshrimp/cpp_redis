#include <cstdint>
#include <fstream>
#include <iostream>

#include "snapshotter.hpp"

Snapshotter::Snapshotter(std::shared_ptr<Storage> storage)
    : storage_(storage) {}

bool Snapshotter::save(const std::string &filename, SnapshotFormat format) {
  if (format != SnapshotFormat::CUSTOM) {
    return false;
  }

  std::ofstream outfile(filename, std::ofstream::binary);
  if (!outfile.is_open()) {
    std::cerr << "Failed to open file for writing: " << filename << std::endl;
    return false;
  }

  auto writer = [&](const std::string &key, const std::string &value) {
    if (!outfile)
      return;

    uint32_t key_len = key.length();
    uint32_t value_len = value.length();
    outfile.write(reinterpret_cast<const char *>(&key_len), sizeof(uint32_t));
    outfile.write(key.c_str(), key.length());
    outfile.write(reinterpret_cast<const char *>(&value_len), sizeof(uint32_t));
    outfile.write(value.c_str(), value.length());
  };
  storage_->visitAll(writer);

  if (!outfile) {
    std::cerr << "ERROR: could not write everything to file: " << filename
              << std::endl;
    return false;
  }
  outfile.close();
  return true;
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

  std::vector<std::string> raw_kvpairs;
  uint32_t str_len;
  while (infile.read(reinterpret_cast<char *>(&str_len), sizeof(uint32_t))) {
    std::string str(str_len, '\0');
    if (infile.read(&str[0], str_len)) {
      raw_kvpairs.push_back(str);
    } else {
      std::cerr << "Error: Unexpected eof or read error." << std::endl;
      break;
    }
  }

  storage_->setPairs(raw_kvpairs);
  return true;
}
