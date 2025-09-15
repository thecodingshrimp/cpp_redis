#ifndef SNAPSHOTTER_HPP
#define SNAPSHOTTER_HPP

#include "storage.hpp"
#include <cstdint>
#include <fstream>

static const uint8_t MAP = 0;
static const uint8_t HMAP = 1;
static const uint8_t LIST = 3;
enum class SnapshotFormat { CUSTOM, CSV, JSON };

static const std::unordered_map<std::string, SnapshotFormat> lookup{
    {"custom", SnapshotFormat::CUSTOM},
    {"json", SnapshotFormat::JSON},
    {"csv", SnapshotFormat::CSV}};

bool write_uint8(std::ofstream &outfile, const uint8_t &c);
bool read_uint8(std::ifstream &infile, uint8_t &c);
bool write_uint32(std::ofstream &outfile, const uint32_t &number);
bool read_uint32(std::ifstream &infile, uint32_t &number);
bool write_string(std::ofstream &outfile, const std::string &str);
bool read_string(std::ifstream &infile, std::string &str);

class Snapshotter {
public:
  explicit Snapshotter(const std::shared_ptr<Storage> storage);

  bool save(const std::string &filename, SnapshotFormat &format);
  bool load(const std::string &filename, SnapshotFormat &format);

private:
  std::shared_ptr<Storage> storage_;
};

#endif
