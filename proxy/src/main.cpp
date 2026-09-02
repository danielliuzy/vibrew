#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "llm.h"
#include "protocol.h"

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

  proto::Frame frame;

  while (true) {
    int conn = accept(fd, nullptr, nullptr);

    if (conn == -1) {
      perror("accept");
      close(fd);
      return EXIT_FAILURE;
    }
    std::cout << "client connected\n";

    while (true) {
      const auto read_result = proto::read_frame(conn, frame);
      if (read_result == proto::ReadResult::Eof) {
        break;
      }
      if (read_result != proto::ReadResult::Ok) {
        std::cerr << "read_frame error " << static_cast<int>(read_result)
                  << '\n';
        break;
      }
      const auto write_result = proto::write_frame(
          conn, frame.type, reinterpret_cast<const char*>(frame.payload.data()),
          frame.payload.size());
      if (write_result != proto::WriteResult::Ok) {
        std::cerr << "write_frame error " << static_cast<int>(write_result)
                  << '\n';
        break;
      }
    }
    close(conn);
    std::cout << "client disconnected\n";
  }

  close(fd);

  return 0;
}