#pragma once

#include <cstdint>
#include <mutex>
#include <string>

class BotSettings {
public:
  BotSettings(const std::string& default_model, uint64_t default_channel_id, const std::string& filepath);

  std::string get_model() const;
  void set_model(const std::string& model);

  uint64_t get_allowed_channel_id() const;
  void set_allowed_channel_id(uint64_t channel_id);

  bool is_channel_allowed(uint64_t channel_id) const;

private:
  mutable std::mutex mutex_;
  std::string filepath_;
  std::string model_;
  uint64_t allowed_channel_id_ = 0;

  void load();
  void save() const;
};
