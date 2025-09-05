#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "parser.hpp"
#include "storage.hpp"
#include "snapshotter.hpp"

class CommandHandler {
public:
  explicit CommandHandler(std::shared_ptr<Storage> storage);

  std::string handle(const Command& cmd);

private:
  std::shared_ptr<Storage> storage_;
  std::unique_ptr<Snapshotter> snapshotter_;
};

#endif
