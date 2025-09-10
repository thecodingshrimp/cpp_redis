#include <string>
#include <unistd.h>

#include "command_handler.hpp"
#include "snapshotter.hpp"

CommandHandler::CommandHandler(std::shared_ptr<Storage> storage)
    : storage_(storage) {
  snapshotter_ = std::make_unique<Snapshotter>(storage);
};

std::string CommandHandler::handle(const Command &cmd) {
  if (cmd.name == "SET") {
    if (cmd.args.size() != 2) {
      return "ERROR: wrong number of arguments for SET command\n";
    }

    storage_->set(cmd.args[0], cmd.args[1]);
    return "OK\n";
  } else if (cmd.name == "GET") {
    if (cmd.args.size() != 1) {
      return "ERROR: wrong number of arguments for GET command\n";
    }

    auto value = storage_->get(cmd.args[0]);
    if (value) {
      return *value + "\n";
    }
    return "-1\n";
  } else if (cmd.name == "DEL") {
    if (cmd.args.size() != 1) {
      return "ERROR: wrong number of arguments for DEL command\n";
    }

    bool deleted = storage_->del(cmd.args[0]);

    return std::to_string(deleted) + "\n";
  } else if (cmd.name == "HSET") {
    if (cmd.args.size() != 3) {
      return "ERROR: wrong number of arguments for HSET command\n";
    }

    storage_->hset(cmd.args[0], cmd.args[1], cmd.args[2]);
    return "OK\n";
  } else if (cmd.name == "HGET") {
    if (cmd.args.size() != 2) {
      return "ERROR: wrong number of arguments for HGET command\n";
    }

    auto value = storage_->hget(cmd.args[0], cmd.args[1]);
    if (value) {
      return *value + "\n";
    }
    return "-1\n";
  } else if (cmd.name == "HDEL") {
    if (cmd.args.size() != 2) {
      return "ERROR: wrong number of arguments for HDEL command\n";
    }

    bool deleted = storage_->hdel(cmd.args[0], cmd.args[1]);

    return std::to_string(deleted) + "\n";
  } else if (cmd.name == "SAVE") {
    if (cmd.args.size() != 2) {
      return "ERROR: wrong number of arguments for SAVE command\n";
    }

    auto it = lookup.find(cmd.args[1]);
    if (it == lookup.end()) {
      return "-1\n";
    }
    SnapshotFormat format = it->second;

    auto value = snapshotter_->save(cmd.args[0], format);

    if (!value) {
      return "-1\n";
    }
    return "OK\n";
  } else if (cmd.name == "LOAD") {
    if (cmd.args.size() != 2) {
      return "ERROR: wrong number of arguments for LOAD command\n";
    }

    auto it = lookup.find(cmd.args[1]);
    if (it == lookup.end()) {
      return "-1\n";
    }
    SnapshotFormat format = it->second;

    auto value = snapshotter_->load(cmd.args[0], format);
    if (value) {
      return "OK\n";
    }

    return "-1\n";
  } else {
    return "ERROR: unknown command\n";
  }
}
