#include "llm.h"

#include <httplib.h>

#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

LlmResult ask(const std::string &prompt) {
  const char *key = std::getenv("OPENAI_API_KEY");
  if (!key) {
    return {false, "", "Invalid api key"};
  }

  nlohmann::json msg;
  msg["role"] = "user";
  msg["content"] = prompt;

  nlohmann::json body;
  body["model"] = "gpt-5.6-sol";
  body["messages"] = nlohmann::json::array({msg});

  httplib::SSLClient cli("api.openai.com");

  httplib::Headers headers{{"Authorization", std::string("Bearer ") + key}};

  const auto res = cli.Post("/v1/chat/completions", headers, body.dump(),
                            "application/json");

  if (!res) {
    return {false, "", httplib::to_string(res.error())};
  }

  if (res->status != 200) {
    return {
        false,
        "",
        "HTTP " + std::to_string(res->status),
    };
  }

  const auto parsed = nlohmann::json::parse(res->body, nullptr, false);

  if (parsed.is_discarded()) {
    return {false, "", "invalid json"};
  }

  try {
    const auto answer =
        parsed.at("/choices/0/message/content"_json_pointer).get<std::string>();
    std::cerr << answer << '\n';
    return {true, answer, ""};
  } catch (const nlohmann::json::exception &e) { return {false, "", e.what()}; }
}