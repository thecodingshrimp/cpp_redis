#ifndef SNAPSHOTTER_HPP
#define SNAPSHOTTER_HPP

#include "storage.hpp"

enum class SnapshotFormat { CUSTOM, CSV, JSON };

static const std::unordered_map<std::string, SnapshotFormat> lookup{
    {"custom", SnapshotFormat::CUSTOM},
    {"json", SnapshotFormat::JSON},
    {"csv", SnapshotFormat::CSV}};

class Snapshotter {
 public:
  explicit Snapshotter(const std::shared_ptr<Storage> storage);

  bool save(const std::string& filename, SnapshotFormat format);
  bool load(const std::string& filename, SnapshotFormat format);

 private:
  std::shared_ptr<Storage> storage_;
};

#endif
