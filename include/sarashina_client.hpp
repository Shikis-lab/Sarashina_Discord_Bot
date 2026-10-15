#pragma once

#include "config.hpp"
#include <string>

class SarashinaClient {
public:
  explicit SarashinaClient(const Config& config);
  ~SarashinaClient() = default;

  // Sends the prompt to Sarashina AI and returns the response text or an error message.
  std::string send_prompt(const std::string& prompt, const std::string& model = "");

private:
  Config config_;
};
