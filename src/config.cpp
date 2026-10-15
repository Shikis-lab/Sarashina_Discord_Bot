#include "config.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

void load_env_file(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    // Trim whitespace
    const auto first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || line[first] == '#') {
      continue;
    }
    const auto last = line.find_last_not_of(" \t\r\n");
    const std::string trimmed = line.substr(first, (last - first + 1));

    const auto eq_pos = trimmed.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    std::string key = trimmed.substr(0, eq_pos);
    std::string val = trimmed.substr(eq_pos + 1);

    // Trim key and val
    const auto k_last = key.find_last_not_of(" \t");
    if (k_last != std::string::npos) {
      key = key.substr(0, k_last + 1);
    }
    const auto v_first = val.find_first_not_of(" \t");
    if (v_first != std::string::npos) {
      val = val.substr(v_first);
    }
    // Remove quotes if present
    if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') ||
                            (val.front() == '\'' && val.back() == '\''))) {
      val = val.substr(1, val.size() - 2);
    }

    // Don't overwrite if already set in environment
    if (std::getenv(key.c_str()) == nullptr) {
#if defined(_WIN32)
      _putenv_s(key.c_str(), val.c_str());
#else
      setenv(key.c_str(), val.c_str(), 0);
#endif
    }
  }
}

std::string get_env_or_default(const char* name, const std::string& default_value = "") {
  const char* val = std::getenv(name);
  if (val != nullptr && *val != '\0') {
    return std::string(val);
  }
  return default_value;
}

} // namespace

Config Config::from_env() {
  load_env_file(".env");

  Config cfg;
  cfg.discord_bot_token = get_env_or_default("DISCORD_BOT_TOKEN");
  cfg.sarashina_api_key = get_env_or_default("SARASHINA_API_KEY");
  cfg.sarashina_model = get_env_or_default("SARASHINA_MODEL", "sarashina3-mini");
  cfg.sarashina_api_url = get_env_or_default("SARASHINA_API_URL", "https://api.sarashina.ai/v1/chat/completions");
  cfg.system_prompt = get_env_or_default("SARASHINA_SYSTEM_PROMPT", "");

  const std::string channel_str = get_env_or_default("ALLOWED_CHANNEL_ID", "0");
  try {
    cfg.allowed_channel_id = std::stoull(channel_str);
  } catch (...) {
    cfg.allowed_channel_id = 0;
  }

  return cfg;
}
