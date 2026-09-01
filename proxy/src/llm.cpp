#include "llm.h"

#include <httplib.h>

#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

namespace {
constexpr const char* kApiKeyEnv = "OPENROUTER_API_KEY";
constexpr const char* kHost = "openrouter.ai";
constexpr const char* kCompletionsPath = "/api/v1/chat/completions";
constexpr const char* kModel = "anthropic/claude-haiku-4.5";
constexpr const char* kContentPath = "/choices/0/message/content";
}  // namespace

LlmResult ask(const std::string& prompt) {
  const char* key = std::getenv(kApiKeyEnv);
  if (!key) {
    std::cerr << "no key in env\n";
    return {false, "", "Invalid api key"};
  }

  nlohmann::json msg;
  msg["role"] = "user";
  msg["content"] = prompt;

  nlohmann::json body;
  body["model"] = "anthropic/claude-haiku-4.5";
  body["messages"] = nlohmann::json::array({msg});

  httplib::SSLClient cli(kHost);

  httplib::Headers headers{{"Authorization", std::string("Bearer ") + key}};

  const auto res =
      cli.Post(kCompletionsPath, headers, body.dump(), "application/json");

  if (!res) {
    std::cerr << "request failed\n";
    return {false, "", httplib::to_string(res.error())};
  }

  if (res->status != 200) {
    std::cerr << "status is " << res->status << '\n';
    return {
        false,
        "",
        "HTTP " + std::to_string(res->status),
    };
  }

  const auto parsed = nlohmann::json::parse(res->body, nullptr, false);

  if (parsed.is_discarded()) {
    std::cerr << "invalid json\n";
    return {false, "", "invalid json"};
  }

  try {
    const auto answer = parsed.at(nlohmann::json::json_pointer(kContentPath))
                            .get<std::string>();
    std::cerr << answer << '\n';
    return {true, answer, ""};
  } catch (const nlohmann::json::exception& e) {
    return {false, "", e.what()};
  }
}