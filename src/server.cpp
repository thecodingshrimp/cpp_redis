#include "server.hpp"
#include "storage.hpp"

#include <cstddef>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

struct uc {
  int uc_fd;
  char *uc_addr;
};

std::vector<uc> users;

int conn_idx(int fd) {
  for (int idx = 0; idx < users.size(); ++idx) {
    if (users[idx].uc_fd == fd) {
      return idx;
    }
  }
  return -1;
}

/* add a new connection storing the IP address */
int conn_add(int fd) {
  uint uidx;
  if (fd < 1) {
    return -1;
  }
  if (users.size() >= USER_AMOUNT) {
    // TODO error handling too many concurrent clients
    close(fd);
    return -1;
  }
  uc new_client = {fd, 0};
  users.push_back(std::move(new_client));
  return 0;
}

/* remove a connection and close it's fd */
int conn_delete(int fd) {
  int uidx;
  if (fd < 1)
    return -1;
  if ((uidx = conn_idx(fd)) == -1) {
    return -1;
  }
  users.erase(users.begin() + uidx);
  /* free(users[uidx].uc_addr); */
  return close(fd);
}

DatabaseServer::DatabaseServer(int port) : port_(port) {
  storage_ = std::make_shared<Storage>();
  commandHandler_ = std::make_unique<CommandHandler>(storage_);
}

// kevent thx to https://eradman.com/posts/kqueue-tcp.html
void DatabaseServer::run() {
  users.reserve(USER_AMOUNT);
  addrinfo *address;
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_socktype = SOCK_STREAM;
  int error =
      getaddrinfo("127.0.0.1", std::to_string(port_).c_str(), &hints, &address);

  if (error) {
    std::cerr << "getaddrinfo: " << gai_strerror(error) << std::endl;
    return;
  }

  int server_fd =
      socket(address->ai_family, address->ai_socktype, address->ai_protocol);
  if (server_fd == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return;
  }

  if (bind(server_fd, address->ai_addr, address->ai_addrlen) < 0) {
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

  struct kevent event;
  struct kevent tevent[EVENT_AMOUNT];
  int kq, nev;

  kq = kqueue();
  if (kq < 0) {
    std::cerr << "kqueue() failed" << std::endl;
    return;
  }

  EV_SET(&event, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(kq, &event, 1, NULL, 0, NULL) < 0) {
    std::cerr << "kevent failed" << std::endl;
    return;
  }

  int fd;
  sockaddr_storage addr;
  socklen_t socklen = sizeof(addr);
  while (true) {
    nev = kevent(kq, NULL, 0, tevent, EVENT_AMOUNT, NULL);

    if (nev < 1) {
      std::cerr << "kevent wait failed" << std::endl;
      return;
    }

    for (int i = 0; i < nev; ++i) {
      if (tevent[i].flags & EV_ERROR) {
        std::cerr << "event error: " << event.data << std::endl;
        return;
      } else if (tevent[i].flags & EV_EOF) {
        std::cerr << "client disconnected" << std::endl;
        fd = tevent[i].ident;
        EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        if (kevent(kq, &event, 1, NULL, 0, NULL) < 0) {
          std::cout << "could not disconnect client" << std::endl;
          return;
        }
        conn_delete(fd);
      } else if (tevent[i].ident == server_fd) {
        fd = accept(tevent[i].ident, reinterpret_cast<struct sockaddr *>(&addr),
                    &socklen);
        if (fd == -1) {
          std::cerr << "error while client accept" << std::endl;
          return;
        }
        if (conn_add(fd) == 0) {
          EV_SET(&event, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
          if (kevent(kq, &event, 1, NULL, 0, NULL) == -1) {
            std::cerr << "could not read stuff" << std::endl;
            return;
          }
          const std::string helloworld = "welcome!\n";
          send(fd, helloworld.c_str(), helloworld.length(), 0);
        } else {
          printf("connection refused\n");
          close(fd);
        }
      } else if (tevent[i].filter == EVFILT_READ) {
        handle_client(tevent[i].ident);
      }
    }
  }
}

void DatabaseServer::handle_client(int client_socket) {
  char buffer[1024] = {0};

  int bytes_read = read(client_socket, buffer, 1024);
  if (bytes_read <= 0) {
    std::cout << "Client disconnected" << std::endl;
    return;
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
