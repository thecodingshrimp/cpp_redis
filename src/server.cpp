#include "server.hpp"
#include "storage.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
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

void set_non_blocking(int sock) {
  if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
    perror("fcntl F_SETFL");
  }
}

/* add a new connection storing the IP address */
int DatabaseServer::conn_add(int fd) {
  uint uidx;
  if (fd < 1) {
    return -1;
  }
  if (users_.size() >= USER_AMOUNT) {
    // TODO error handling too many concurrent clients
    close(fd);
    return -1;
  }
  uc new_client = {fd, 0};
  users_.emplace(fd, std::move(new_client));
  return 0;
}

/* remove a connection and close it's fd */
int DatabaseServer::conn_delete(int fd) {
  int uidx;
  if (fd < 1)
    return -1;
  auto it = users_.find(fd);
  if (it == users_.end()) {
    return -1;
  }
  users_.erase(it);
  /* free(users_[uidx].uc_addr); */
  return close(fd);
}

DatabaseServer::DatabaseServer(int port) : port_(port) {
  storage_ = std::make_shared<Storage>();
  commandHandler_ = std::make_unique<CommandHandler>(storage_);
}

// kevent thx to https://eradman.com/posts/kqueue-tcp.html
void DatabaseServer::run() {
  users_.reserve(USER_AMOUNT);
  addrinfo *address;
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_flags = AI_PASSIVE;
  // SOCK_NONBLOCK is not defined on mac machines according to
  // https://forum.systemcrafters.net/t/a-non-guix-guile-setup/115/7
  // have to use fcntl since O_NONBLOCK does not work here.
  hints.ai_socktype = SOCK_STREAM;
  int error =
      getaddrinfo("127.0.0.1", std::to_string(port_).c_str(), &hints, &address);

  if (error) {
    std::cerr << "getaddrinfo: " << gai_strerror(error) << std::endl;
    return;
  }

  int server_fd =
      socket(address->ai_family, address->ai_socktype, address->ai_protocol);
  // use fcntl to make server_fd non-blocking.
  set_non_blocking(server_fd);
  if (server_fd == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return;
  }

  if (bind(server_fd, address->ai_addr, address->ai_addrlen) < 0) {
    std::cerr << "Bind failed" << std::endl;
    close(server_fd);
    return;
  }

  if (listen(server_fd, SOMAXCONN) < 0) {
    std::cerr << "Listen failed" << std::endl;
    close(server_fd);
    return;
  }
  std::cout << "Server listening on port " << port_ << std::endl;

  struct kevent event;
  struct kevent tevent[EVENT_AMOUNT];
  int nev;

  kq_ = kqueue();
  if (kq_ < 0) {
    std::cerr << "kqueue() failed" << std::endl;
    return;
  }

  EV_SET(&event, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(kq_, &event, 1, NULL, 0, NULL) < 0) {
    std::cerr << "kevent failed" << std::endl;
    return;
  }

  int fd;
  sockaddr_storage addr;
  socklen_t socklen = sizeof(addr);
  while (true) {
    nev = kevent(kq_, NULL, 0, tevent, EVENT_AMOUNT, NULL);

    if (nev < 1) {
      std::cerr << "kevent wait failed" << std::endl;
      return;
    }

    for (int i = 0; i < nev; ++i) {
      fd = tevent[i].ident;

      if (tevent[i].flags & EV_ERROR) {
        std::cerr << "event error: " << event.data << std::endl;
        continue;
      }

      if (tevent[i].flags & EV_EOF) {
        std::cerr << "client " << fd << " disconnected" << std::endl;
        struct kevent changelist[2];
        EV_SET(&changelist[0], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        EV_SET(&changelist[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        if (kevent(kq_, changelist, 2, NULL, 0, NULL) < 0) {
          perror("EV_EOF");
        }
        conn_delete(fd);
        continue;
      }

      if (fd == server_fd) {
        // macos does not know accept4, so no O_NONBLOCK OR SOCK_NONBLOCK
        fd = accept(tevent[i].ident, reinterpret_cast<struct sockaddr *>(&addr),
                    &socklen);
        if (fd < 0) {
          perror("accept");
          continue;
        }
        // need to set non blocking explicitly.
        set_non_blocking(fd);
        if (conn_add(fd) == 0) {
          users_[fd].write_buffer = "Welcome\n";
          struct kevent changelist[2];
          EV_SET(&changelist[0], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
          // enable for the welcome message
          EV_SET(&changelist[1], fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
          if (kevent(kq_, &changelist[0], 2, NULL, 0, NULL) == -1) {
            perror("onboard client");
            conn_delete(fd);
          }
        } else {
          printf("connection refused\n");
          close(fd);
        }
        continue;
      }
      if (tevent[i].filter & EVFILT_WRITE) {
        handle_client_write(tevent[i].ident);
      }
      if (tevent[i].filter & EVFILT_READ) {
        handle_client_read(tevent[i].ident);
      }
    }
  }
}

void DatabaseServer::handle_client_write(int client_socket) {
  auto client = users_.find(client_socket);
  if (client == users_.end()) {
    perror("find user");
    return;
  }

  auto &client_info = client->second;

  auto sent = send(client_socket, client_info.write_buffer.c_str(),
                   client_info.write_buffer.length(), 0);

  if (sent < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // buffer is full. try again later.
      return;
    }
    perror("send");
    conn_delete(client_socket);
    return;
  }

  if (sent < client_info.write_buffer.length()) {
    client_info.write_buffer.erase(0, sent);
  } else {
    client_info.write_buffer.clear();
  }

  if (client_info.write_buffer.empty()) {
    struct kevent event;
    EV_SET(&event, client_socket, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
    if (kevent(kq_, &event, 1, NULL, 0, NULL) < 0) {
      std::cerr << "disable kevent write" << std::endl;
      conn_delete(client_socket);
    }
  }
}

void DatabaseServer::handle_client_read(int client_socket) {
  auto client = users_.find(client_socket);
  if (client == users_.end()) {
    perror("find user");
    return;
  }
  char buffer[1024] = {0};

  int bytes_read = recv(client_socket, buffer, 1024, MSG_DONTWAIT);
  if (bytes_read <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    } else if (bytes_read == 0) {
      std::cout << "Client " << client_socket << " disconnected (read 0 bytes)."
                << std::endl;
    } else {
      perror("read");
    }
    conn_delete(client_socket);
    return;
  }

  users_[client_socket].read_buffer.append(buffer, bytes_read);

  while (true) {
    auto pos = users_[client_socket].read_buffer.find('\n');
    if (pos == std::string::npos) {
      return;
    }

    std::string command = users_[client_socket].read_buffer.substr(0, pos);
    users_[client_socket].read_buffer.erase(0, pos + 1);

    if (command.empty()) {
      continue;
    }
    auto command_opt = parser_.parse(command);

    std::string response;
    if (command_opt) {
      response = commandHandler_->handle(*command_opt);
    } else {
      response = "ERROR: invalid command\n";
    }

    struct kevent event;
    users_[client_socket].write_buffer.append(std::move(response));
    EV_SET(&event, client_socket, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
    if (kevent(kq_, &event, 1, NULL, 0, NULL) < 0) {
      std::cerr << "kevent write failed" << std::endl;
      return;
    }
  }
}
