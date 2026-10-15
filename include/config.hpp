#pragma once

#include <cstdint>
#include <string>

struct Config {
  std::string discord_bot_token;
  std::string sarashina_api_key;
  std::string sarashina_model = "sarashina3-mini";
  std::string sarashina_api_url = "https://api.sarashina.ai/v1/chat/completions";
  std::string system_prompt;
  uint64_t allowed_channel_id = 0; // 0 means not restricted, will respond to mentions in all channels

  static Config from_env();
};
