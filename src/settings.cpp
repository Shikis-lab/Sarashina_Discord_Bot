#include "settings.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

BotSettings::BotSettings(const std::string& default_model, uint64_t default_channel_id, const std::string& filepath)
    : filepath_(filepath), model_(default_model), allowed_channel_id_(default_channel_id) {
  load();
}

std::string BotSettings::get_model() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return model_;
}

void BotSettings::set_model(const std::string& model) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    model_ = model;
  }
  save();
}

uint64_t BotSettings::get_allowed_channel_id() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return allowed_channel_id_;
}

void BotSettings::set_allowed_channel_id(uint64_t channel_id) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    allowed_channel_id_ = channel_id;
  }
  save();
}

bool BotSettings::is_channel_allowed(uint64_t channel_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return allowed_channel_id_ != 0 && allowed_channel_id_ == channel_id;
}

void BotSettings::load() {
  if (filepath_.empty() || !std::filesystem::exists(filepath_)) {
    return;
  }

  try {
    std::ifstream file(filepath_);
    if (!file.is_open()) return;

    nlohmann::json j;
    file >> j;

    std::lock_guard<std::mutex> lock(mutex_);
    if (j.contains("model") && j["model"].is_string()) {
      model_ = j["model"].get<std::string>();
    }
    if (j.contains("allowed_channel_id") && j["allowed_channel_id"].is_number_unsigned()) {
      allowed_channel_id_ = j["allowed_channel_id"].get<uint64_t>();
    }
    std::cout << "[Settings] 設定ファイルを読み込みました: " << filepath_
              << " (モデル: " << model_ << ", チャンネル: " << allowed_channel_id_ << ")" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "[Settings] 設定ファイル読み込みエラー: " << e.what() << std::endl;
  }
}

void BotSettings::save() const {
  if (filepath_.empty()) {
    return;
  }

  try {
    const auto dir = std::filesystem::path(filepath_).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
      std::filesystem::create_directories(dir);
    }

    nlohmann::json j;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      j["model"] = model_;
      j["allowed_channel_id"] = allowed_channel_id_;
    }

    std::ofstream file(filepath_);
    if (file.is_open()) {
      file << j.dump(2);
      std::cout << "[Settings] 設定ファイルを保存しました: " << filepath_ << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "[Settings] 設定ファイル保存エラー: " << e.what() << std::endl;
  }
}
