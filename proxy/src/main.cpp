#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd == -1) {
    perror("socket");
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

  std::cout << "Done\n";
  close(fd);

  return 0;
}