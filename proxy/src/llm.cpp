#include "llm.h"

#include <httplib.h>

#include <cstdlib>
#include <nlohmann/json.hpp>
#include <string>

LlmResult ask(const std::string& prompt) {
  const char* key = std::getenv("OPENAI_API_KEY");
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

  return {true, prompt, ""};
}