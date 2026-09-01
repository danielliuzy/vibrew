#include "llm.h"

#include <httplib.h>

#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

LlmResult ask(const std::string& prompt) {
  const char* key = std::getenv("OPENROUTER_API_KEY");
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

  httplib::SSLClient cli("openrouter.ai");

  httplib::Headers headers{{"Authorization", std::string("Bearer ") + key}};

  const auto res = cli.Post("/api/v1/chat/completions", headers, body.dump(),
                            "application/json");

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
    const auto answer =
        parsed.at("/choices/0/message/content"_json_pointer).get<std::string>();
    std::cerr << answer << '\n';
    return {true, answer, ""};
  } catch (const nlohmann::json::exception& e) {
    return {false, "", e.what()};
  }
}