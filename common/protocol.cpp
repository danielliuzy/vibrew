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

ReadResult read_frame(int fd, Frame& out) {
  char header[kHeaderSize];
  const auto header_result = read_exactly(fd, header, kHeaderSize);
  if (header_result != ReadResult::Ok) {
    return header_result;
  }
  const auto* bytes = reinterpret_cast<const unsigned char*>(header);
  const uint32_t len = uint32_t{bytes[0]} | uint32_t{bytes[1]} << 8 |
                       uint32_t{bytes[2]} << 16 | uint32_t{bytes[3]} << 24;
  if (len > kMaxPayload) {
    return ReadResult::Error;
  }
  const auto type = static_cast<MsgType>(bytes[4]);
  out.payload.resize(len);
  const auto read_result =
      read_exactly(fd, reinterpret_cast<char*>(out.payload.data()), len);
  if (read_result == ReadResult::Eof) {
    return ReadResult::Truncated;
  }
  if (read_result != ReadResult::Ok) {
    return read_result;
  }
  out.type = type;
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

WriteResult write_frame(int fd, MsgType type, const char* buf, size_t length) {
  if (length > kMaxPayload) {
    return WriteResult::Error;
  }

  unsigned char header[kHeaderSize];
  const uint32_t len = static_cast<uint32_t>(length);
  header[0] = static_cast<unsigned char>(len & 0xFF);
  header[1] = static_cast<unsigned char>((len >> 8) & 0xFF);
  header[2] = static_cast<unsigned char>((len >> 16) & 0xFF);
  header[3] = static_cast<unsigned char>((len >> 24) & 0xFF);
  header[4] = static_cast<unsigned char>(type);
  const auto header_result =
      write_all(fd, reinterpret_cast<const char*>(header), kHeaderSize);
  if (header_result != WriteResult::Ok) {
    return header_result;
  }
  return write_all(fd, buf, length);
}

}  // namespace proto