#ifndef SERVER_HPP
#define SERVER_HPP

#include "command_handler.hpp"
#include "parser.hpp"
#include "storage.hpp"
#include <cstdint>
#include <unordered_map>

struct uc {
  int uc_fd;
  char *uc_addr;
  std::string buffer;
};

static const uint16_t EVENT_AMOUNT = 256;
static const uint16_t USER_AMOUNT = 256;
class DatabaseServer {
public:
  explicit DatabaseServer(int port);

  void run();

private:
  int conn_add(int fd);
  int conn_delete(int fd);
  void handle_client(int client_socket);
  int port_;
  std::shared_ptr<Storage> storage_;
  std::unique_ptr<CommandHandler> commandHandler_;
  Parser parser_;
  std::unordered_map<int, uc> users_;
};

#endif
