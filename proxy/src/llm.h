#pragma once

#include <string>

struct LlmResult {
  bool ok;
  std::string text;
  std::string error;
};

LlmResult ask(const std::string& prompt);