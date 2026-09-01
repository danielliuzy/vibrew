#pragma once
#include <cstddef>

namespace proto {

enum class ReadResult { Ok, Eof, Truncated, Error };
[[nodiscard]] ReadResult read_exactly(int fd, char* buf, size_t length);

enum class WriteResult { Ok, Closed, Error };
[[nodiscard]] WriteResult write_all(int fd, const char* buf, size_t length);

}  // namespace proto