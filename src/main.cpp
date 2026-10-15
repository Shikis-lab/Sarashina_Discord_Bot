#include "config.hpp"
#include "sarashina_client.hpp"
#include "settings.hpp"

#include <dpp/dpp.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <thread>
#include <vector>

namespace {

// Split message into chunks below Discord's 2000 character limit
std::vector<std::string> chunk_text(const std::string& text, size_t chunk_size = 1900) {
  std::vector<std::string> chunks;
  if (text.empty()) {
    chunks.push_back("");
    return chunks;
  }

  size_t start = 0;
  while (start < text.length()) {
    size_t length = std::min(chunk_size, text.length() - start);

    // If chunking in the middle, try to break at a newline if possible
    if (start + length < text.length()) {
      const auto last_newline = text.rfind('\n', start + length);
      if (last_newline != std::string::npos && last_newline > start + (chunk_size / 2)) {
        length = last_newline - start + 1;
      }
    }

    chunks.push_back(text.substr(start, length));
    start += length;
  }

  return chunks;
}

// Strip bot mentions from the prompt text
std::string clean_prompt(const std::string& raw_content, uint64_t bot_id) {
  std::string id_str = std::to_string(bot_id);
  std::regex mention_pattern("<@!?" + id_str + ">");
  std::string cleaned = std::regex_replace(raw_content, mention_pattern, "");

  // Trim whitespace
  const auto first = cleaned.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }
  const auto last = cleaned.find_last_not_of(" \t\r\n");
  return cleaned.substr(first, (last - first + 1));
}

bool is_bot_mentioned(const dpp::message_create_t& event, uint64_t bot_id) {
  for (const auto& user : event.msg.mentions) {
    if (user.first.id == bot_id) {
      return true;
    }
  }

  std::string id_str = std::to_string(bot_id);
  if (event.msg.content.find("<@" + id_str + ">") != std::string::npos ||
      event.msg.content.find("<@!" + id_str + ">") != std::string::npos) {
    return true;
  }

  return false;
}

void reply_ephemeral(const dpp::slashcommand_t& event, const std::string& text) {
  event.reply(dpp::message(text).set_flags(dpp::m_ephemeral));
}

} // namespace

int main() {
  // Disable stdout buffering for Docker logs
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);

  Config config = Config::from_env();

  if (config.discord_bot_token.empty()) {
    std::cerr << "[エラー] DISCORD_BOT_TOKEN が設定されていません。" << std::endl;
    return 1;
  }

  if (config.sarashina_api_key.empty()) {
    std::cerr << "[警告] SARASHINA_API_KEY が設定されていません。API呼び出し時にエラーになります。" << std::endl;
  }

  auto settings = std::make_shared<BotSettings>(
      config.sarashina_model,
      config.allowed_channel_id,
      "data/settings.json"
  );

  std::cout << "Sarashina Discord Bot 起動中..." << std::endl;
  std::cout << "現在モデル: " << settings->get_model() << std::endl;
  std::cout << "連携チャンネル: " << settings->get_allowed_channel_id() << std::endl;

  dpp::cluster bot(config.discord_bot_token, dpp::i_default_intents | dpp::i_message_content);

  bot.on_log([](const dpp::log_t& event) {
    std::cout << "[" << dpp::utility::loglevel(event.severity) << "] " << event.message << std::endl;
  });

  bot.on_ready([&bot](const dpp::ready_t& event) {
    std::cout << "Botログイン完了: " << bot.me.username << "#" << bot.me.discriminator << " (" << bot.me.id << ")" << std::endl;

    if (dpp::run_once<struct register_bot_commands>()) {
      // /set_channel [channel]
      dpp::slashcommand cmd_set_channel("set_channel", "通常メッセージで自動応答するチャンネルを設定します", bot.me.id);
      cmd_set_channel.add_option(dpp::command_option(dpp::co_channel, "channel", "対象のチャンネル（省略時は現在のチャンネル）", false));
      bot.global_command_create(cmd_set_channel);

      // /unset_channel
      dpp::slashcommand cmd_unset_channel("unset_channel", "チャンネル紐付けを解除します（メンション応答のみになります）", bot.me.id);
      bot.global_command_create(cmd_unset_channel);

      // /set_model <model>
      dpp::slashcommand cmd_set_model("set_model", "Sarashina AIのモデルを変更します", bot.me.id);
      dpp::command_option opt_model(dpp::co_string, "model", "使用するモデル", true);
      opt_model.add_choice(dpp::command_option_choice("sarashina3-mini (標準対話・高精度)", "sarashina3-mini"));
      opt_model.add_choice(dpp::command_option_choice("sarashina3-nano (軽量・高速)", "sarashina3-nano"));
      cmd_set_model.add_option(opt_model);
      bot.global_command_create(cmd_set_model);

      // /status
      dpp::slashcommand cmd_status("status", "現在のBot設定（モデル、紐付けチャンネル等）を確認します", bot.me.id);
      bot.global_command_create(cmd_status);

      // /help
      dpp::slashcommand cmd_help("help", "Botの使い方と利用可能なコマンド一覧を表示します", bot.me.id);
      bot.global_command_create(cmd_help);
    }
  });

  bot.on_slashcommand([settings](const dpp::slashcommand_t& event) {
    const std::string command = event.command.get_command_name();

    if (command == "set_channel") {
      dpp::snowflake target_channel = event.command.channel_id;
      const auto param = event.get_parameter("channel");
      if (std::holds_alternative<dpp::snowflake>(param)) {
        target_channel = std::get<dpp::snowflake>(param);
      }

      settings->set_allowed_channel_id(target_channel);
      reply_ephemeral(event, "✅ <#" + std::to_string(target_channel) + "> をSarashina AIの自動応答チャンネルに設定しました！\nこのチャンネルではメンションなしで通常発言するだけで自動返信されます。");
      return;
    }

    if (command == "unset_channel") {
      settings->set_allowed_channel_id(0);
      reply_ephemeral(event, "✅ チャンネル紐付けを解除しました。\n今後はどのチャンネルでも `@Bot <質問>` のメンション時のみ応答します。");
      return;
    }

    if (command == "set_model") {
      const std::string selected_model = std::get<std::string>(event.get_parameter("model"));
      settings->set_model(selected_model);
      reply_ephemeral(event, "✅ モデルを **" + selected_model + "** に変更しました！");
      return;
    }

    if (command == "status") {
      const std::string current_model = settings->get_model();
      const uint64_t channel_id = settings->get_allowed_channel_id();

      std::string msg = "📊 **Sarashina Bot 設定ステータス**\n";
      msg += "・**使用中モデル**: `" + current_model + "`\n";
      if (channel_id != 0) {
        msg += "・**自動応答チャンネル**: <#" + std::to_string(channel_id) + ">\n";
      } else {
        msg += "・**自動応答チャンネル**: 未設定 (メンション時のみ全チャンネルで応答)\n";
      }

      reply_ephemeral(event, msg);
      return;
    }

    if (command == "help") {
      std::string msg = "📖 **Sarashina Bot 使い方ガイド**\n\n";
      msg += "国産 LLM「Sarashina AI」と Discord 上で手軽に対話できる Bot です。\n\n";
      msg += "💬 **質問のしかた**\n";
      msg += "・**メンションで質問**: 任意のチャンネルで `@SarashinaKun <質問内容>` と入力して送信します。\n";
      msg += "・**自動応答チャンネルで質問**: 紐付けたチャンネル内では、メンション不要で通常発言するだけで自動返信されます。\n\n";
      msg += "⚙️ **コマンド一覧**\n";
      msg += "・`/help` : この使い方ガイドを表示します。\n";
      msg += "・`/status` : 現在のモデルや紐付けチャンネルの設定状況を確認します。\n";
      msg += "・`/set_channel [channel]` : 指定したチャンネル（省略時は現在のチャンネル）を自動応答に設定します。\n";
      msg += "・`/unset_channel` : 自動応答チャンネルの設定を解除します（メンション応答のみになります）。\n";
      msg += "・`/set_model <model>` : Sarashina AI のモデルを変更します (`sarashina3-mini` / `sarashina3-nano`)。\n";

      reply_ephemeral(event, msg);
      return;
    }

    reply_ephemeral(event, "不明なコマンドです。`/help` でコマンド一覧をご確認ください。");
  });

  bot.on_message_create([&bot, config, settings](const dpp::message_create_t& event) {
    // Ignore messages from bots (including self)
    if (event.msg.author.is_bot()) {
      return;
    }

    const uint64_t bot_id = bot.me.id;
    const bool mentioned = is_bot_mentioned(event, bot_id);
    const bool in_allowed_channel = settings->is_channel_allowed(event.msg.channel_id);

    // Only respond if mentioned OR in the allowed channel
    if (!mentioned && !in_allowed_channel) {
      return;
    }

    const std::string prompt = clean_prompt(event.msg.content, bot_id);
    if (prompt.empty()) {
      dpp::message reply(event.msg.channel_id, "Sarashina AI に質問したい内容を教えてください！\n（使い方確認: `/help`、設定確認: `/status`）");
      reply.set_reference(event.msg.id);
      bot.message_create(reply);
      return;
    }

    // Trigger typing indicator
    bot.channel_typing(event.msg.channel_id);

    const std::string current_model = settings->get_model();

    // Handle Sarashina request asynchronously in a worker thread so the gateway loop is not blocked
    std::thread([&bot, config, prompt, current_model, event]() {
      SarashinaClient client(config);
      std::string answer = client.send_prompt(prompt, current_model);

      auto chunks = chunk_text(answer);
      for (size_t i = 0; i < chunks.size(); ++i) {
        dpp::message msg(event.msg.channel_id, chunks[i]);
        if (i == 0) {
          msg.set_reference(event.msg.id);
        }
        bot.message_create(msg);
      }
    }).detach();
  });

  bot.start(dpp::st_wait);

  return 0;
}
