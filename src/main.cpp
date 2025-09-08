#include "server.hpp"

#include <iostream>

const int DEFAULT_PORT = 3000;

int main(int argc, char *argv[]) {
  int port = DEFAULT_PORT;
  if (argc > 1) {
    try {
      port = std::stoul(argv[1]);
    } catch (const std::exception &e) {
      std::cerr << "Invalid port number. Using default " << DEFAULT_PORT
                << std::endl;
    }
  }

  try {
    DatabaseServer server(port);
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "Server error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
