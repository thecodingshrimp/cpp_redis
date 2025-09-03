#include "command_handler.hpp"

CommandHandler::CommandHandler(std::shared_ptr<Storage> storage) : storage_(storage) {};

std::string CommandHandler::handle(const Command& cmd) {
  // todo check if cmd is defined
  
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
    } else {
      return "-1\n";
    }
  } else if (cmd.name == "DEL") {
    if (cmd.args.size() != 1) {
      return "ERROR: wrong number of arguments for DEL command\n";
    }
    bool deleted = storage_->del(cmd.args[0]);
    return std::to_string(deleted) + "\n";
  } else {
    return "ERROR: unknown command\n";
  }
}