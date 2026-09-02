#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace proto {

enum class MsgType : uint8_t { Hello = 1, TextRequest = 2 };
struct Frame {
  MsgType type;
  std::vector<std::byte> payload;
};

enum class ReadResult { Ok, Eof, Truncated, Error };
enum class WriteResult { Ok, Closed, Error };

inline constexpr size_t kHeaderSize = 5;
inline constexpr uint32_t kMaxPayload = 60 * 1024;

[[nodiscard]] ReadResult read_exactly(int fd, char* buf, size_t length);
[[nodiscard]] ReadResult read_frame(int fd, Frame& out);

[[nodiscard]] WriteResult write_all(int fd, const char* buf, size_t length);
[[nodiscard]] WriteResult write_frame(int fd, MsgType type, const char* buf,
                                      size_t length);

}  // namespace proto