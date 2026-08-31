#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "llm.h"

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  int yes{1};
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    perror("setsockopt");
    close(fd);
    return EXIT_FAILURE;
  }

  struct sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    perror("bind");
    close(fd);
    return EXIT_FAILURE;
  }

  if (listen(fd, SOMAXCONN) == -1) {
    perror("listen");
    close(fd);
    return EXIT_FAILURE;
  }

  int conn = accept(fd, nullptr, nullptr);

  if (conn == -1) {
    perror("accept");
    close(fd);
    return EXIT_FAILURE;
  }

  while (true) {
    char buf[1024];
    ssize_t n = read(conn, buf, sizeof(buf));
    if (n == -1) {
      perror("read");
      close(conn);
      close(fd);
      return EXIT_FAILURE;
    } else if (n > 0) {
      std::cout.write(buf, n);
    } else {
      break;
    }
  }

  std::cout << "closing\n";
  close(conn);
  close(fd);

  return 0;
}