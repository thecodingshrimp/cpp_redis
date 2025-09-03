#ifndef SERVER_HPP
#define SERVER_HPP

#include "storage.hpp"
#include "command_handler.hpp"
#include "parser.hpp"

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