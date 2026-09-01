#include "protocol.h"

#include <sys/socket.h>

#include <cerrno>
#include <cstddef>

namespace proto {

ReadResult read_exactly(int fd, char* buf, size_t length) {
  if (length == 0) {
    return ReadResult::Ok;
  }
  size_t total{0};
  while (total < length) {
    auto received = recv(fd, buf + total, length - total, 0);
    if (received == -1) {
      if (errno == EINTR) {
        continue;
      }
      return ReadResult::Error;
    }
    if (received == 0) {
      break;
    }
    total += received;
  }
  if (total == 0) {
    return ReadResult::Eof;
  } else if (total < length) {
    return ReadResult::Truncated;
  }
  return ReadResult::Ok;
}

WriteResult write_all(int fd, const char* buf, size_t length) {
  if (length == 0) {
    return WriteResult::Ok;
  }

  size_t total{0};
  while (total < length) {
    auto sent = send(fd, buf + total, length - total, 0);

    if (sent == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EPIPE || errno == ECONNRESET) {
        return WriteResult::Closed;
      }
      return WriteResult::Error;
    }
    total += sent;
  }

  return WriteResult::Ok;
}

}  // namespace proto