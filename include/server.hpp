#ifndef SERVER_HPP
#define SERVER_HPP

#include "command_handler.hpp"
#include "parser.hpp"
#include "storage.hpp"
#include <cstdint>

static const uint16_t EVENT_AMOUNT = 256;
static const uint16_t USER_AMOUNT = 256;
class DatabaseServer {
public:
  explicit DatabaseServer(int port);

  void run();

  void handle_client(int client_socket);

private:
  int port_;
  std::shared_ptr<Storage> storage_;
  std::unique_ptr<CommandHandler> commandHandler_;
  Parser parser_;
};

#endif
