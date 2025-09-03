#include "server.hpp"
#include "storage.hpp"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>

DatabaseServer::DatabaseServer(int port) : port_(port) {
  storage_ = std::make_shared<Storage>();
  commandHandler_ = std::make_unique<CommandHandler>(storage_);
}

void DatabaseServer::run() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return;
  }

  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port_);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << "Bind failed" << std::endl;
    close(server_fd);
    return;
  }

  if (listen(server_fd, 256) < 0) {
    std::cerr << "Listen failed" << std::endl;
    close(server_fd);
    return;
  }

  std::cout << "DatabaseServer listening on port " << port_ << std::endl;

  while (true)
  {
    int addrlen = sizeof(address);
    int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
      std::cerr << "Accept failed" << std::endl;
      continue;
    }

    std::thread client_thread(&DatabaseServer::handle_client, this, client_socket);
    client_thread.detach();
  }
}

void DatabaseServer::handle_client(int client_socket) {
  char buffer[1024] = {0};

  while (true) {
    int bytes_read = read(client_socket, buffer, 1024);
    if (bytes_read <= 0) {
      std::cout << "Client disconnected" << std::endl;
      break;
    }

    std::string input(buffer, bytes_read);

    auto command_opt = parser_.parse(input);

    std::string response;
    if (command_opt) {
      response = commandHandler_->handle(*command_opt);
    } else {
      response = "ERROR: invalid command\n";
    }

    send(client_socket, response.c_str(), response.length(), 0);
  }

  close(client_socket);
}