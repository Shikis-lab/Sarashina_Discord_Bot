#include "sarashina_client.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>

namespace {

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t total_size = size * nmemb;
  std::string* str = static_cast<std::string*>(userp);
  str->append(static_cast<char*>(contents), total_size);
  return total_size;
}

} // namespace

SarashinaClient::SarashinaClient(const Config& config) : config_(config) {}

std::string SarashinaClient::send_prompt(const std::string& prompt, const std::string& model) {
  if (config_.sarashina_api_key.empty()) {
    return "[エラー] SARASHINA_API_KEY が設定されていません。";
  }

  const std::string active_model = model.empty() ? config_.sarashina_model : model;

  nlohmann::json messages = nlohmann::json::array();
  if (!config_.system_prompt.empty()) {
    messages.push_back({
      {"role", "system"},
      {"content", config_.system_prompt}
    });
  }
  messages.push_back({
    {"role", "user"},
    {"content", prompt}
  });

  nlohmann::json request_body = {
    {"model", active_model},
    {"messages", messages}
  };

  std::string request_payload = request_body.dump();

  CURL* curl = curl_easy_init();
  if (!curl) {
    return "[エラー] CURLの初期化に失敗しました。";
  }

  std::string response_string;
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  std::string auth_header = "Authorization: Bearer " + config_.sarashina_api_key;
  headers = curl_slist_append(headers, auth_header.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, config_.sarashina_api_url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_payload.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); // 60s timeout
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "SarashinaDiscordBot/1.0 (curl; C++)");

  CURLcode res = curl_easy_perform(curl);

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::ostringstream oss;
    oss << "[エラー] Sarashina API への接続に失敗しました: " << curl_easy_strerror(res);
    return oss.str();
  }

  // Handle non-200 responses
  if (http_code != 200) {
    try {
      auto err_json = nlohmann::json::parse(response_string);
      if (err_json.contains("message")) {
        return "[Sarashina API エラー " + std::to_string(http_code) + "] " + err_json["message"].get<std::string>();
      }
      if (err_json.contains("error")) {
        const auto& err = err_json["error"];
        if (err.is_object() && err.contains("message")) {
          return "[Sarashina API エラー " + std::to_string(http_code) + "] " + err["message"].get<std::string>();
        }
        return "[Sarashina API エラー " + std::to_string(http_code) + "] " + err.dump();
      }
    } catch (...) {
      // Not JSON, return trimmed response string
    }
    return "[Sarashina API エラー " + std::to_string(http_code) + "] " + response_string;
  }

  try {
    auto json_resp = nlohmann::json::parse(response_string);

    if (json_resp.contains("choices") && json_resp["choices"].is_array() && !json_resp["choices"].empty()) {
      const auto& first_choice = json_resp["choices"][0];
      if (first_choice.contains("message") && first_choice["message"].contains("content")) {
        return first_choice["message"]["content"].get<std::string>();
      }
    }

    return "[エラー] 予期しないレスポンス形式です: " + response_string;
  } catch (const std::exception& e) {
    std::ostringstream oss;
    oss << "[エラー] レスポンスの解析に失敗しました (" << e.what() << "): " << response_string;
    return oss.str();
  }
}
