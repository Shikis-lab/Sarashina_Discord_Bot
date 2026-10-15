# Sarashina AI Discord Bot (C++ / Docker)

SB Intuitions の国産 LLM **Sarashina API**（`sarashina3-mini` / `sarashina3-nano`）と連携する Discord Bot です。
C++ (DPP - DPlusPlus) で実装されており、Docker コンテナ環境で軽量・安全に動作します。

## 機能
- **Sarashina AI へのプロンプト送信**: ユーザーの入力を `https://api.sarashina.ai/v1/chat/completions` に転送し、生成結果を返信します。
- **柔軟なトリガー**:
  - **Bot へのメンション**（`@Bot <メッセージ>`）
  - **特定チャンネルとの紐付け**: `ALLOWED_CHANNEL_ID` を指定することで、そのチャンネル内の発言に自動で応答（メンション不要）させることも可能です。
- **Discord 2000文字制限対策**: 回答が長い場合は自動的に分割して送信します。
- **非同期レスポンス & タイピング表示**: API レスポンス待ちの間にタイピングインジケーターを表示し、Discord のゲートウェイスレッドをブロックしません。

---

## 必要条件
- [Docker](https://www.docker.com/) & Docker Compose
- Discord Bot Token
- Sarashina API Key

---

## セットアップ手順

### 1. Discord Bot の準備
1. [Discord Developer Portal](https://discord.com/developers/applications) にアクセスし、アプリケーションを作成します。
2. **Bot** タブでトークンを取得します（Reset Token）。
3. **Privileged Gateway Intents** セクションで **MESSAGE CONTENT INTENT** を **ON** にします（※メッセージ本文を読み取るために必須です）。
4. **OAuth2 > URL Generator** から `bot` スコープを選び、権限（Send Messages, Read Messages/View Channels, Read Message History 等）を付与してサーバーに招待します。

### 2. Sarashina API Key の取得
1. [Sarashina API コンソール](https://chat.sarashina.ai/dashboard/api-keys) にアクセスして API キーを発行します。

### 3. 環境設定
`.env.example` をコピーして `.env` を作成し、各値を入力します。

```bash
cp .env.example .env
```

`.env` の内容例:
```env
# 必須
DISCORD_BOT_TOKEN=your_discord_bot_token_here
SARASHINA_API_KEY=your_sarashina_api_key_here

# 任意
SARASHINA_MODEL=sarashina3-mini
SARASHINA_SYSTEM_PROMPT=あなたは親切で丁寧なアシスタントです。
ALLOWED_CHANNEL_ID=0  # 0: 未指定（メンションのみに反応）。チャンネルIDを指定するとそのチャンネルの全発言に応答
```

---

## 起動方法 (Docker Compose)

### ビルド & バックグラウンド起動
```bash
docker compose up -d --build
```

### ログ確認
```bash
docker compose logs -f bot
```

### 停止
```bash
docker compose down
```

---

## 使い方

### 1. スラッシュコマンド
Discord のチャット欄で `/` を入力して利用できます。設定内容は `./data/settings.json` に保存され、コンテナ再起動後も引き継がれます。

- **`/help`** : Bot の使い方とコマンド一覧を表示します。
- **`/status`** : 現在のモデルと自動応答チャンネルの設定状態を確認します。
- **`/set_channel [channel]`** : 指定したチャンネル（省略時はコマンドを実行した現在のチャンネル）を**自動応答チャンネル**に設定します（メンション不要で通常発言に自動返信）。
- **`/unset_channel`** : 自動応答チャンネルの設定を解除します（メンション応答のみになります）。
- **`/set_model <model>`** : Sarashina AI のモデルを切り替えます（`sarashina3-mini` / `sarashina3-nano`）。

### 2. メッセージ送信
- **メンションで質問**:
  どのチャンネルでも `@SarashinaKun <質問内容>` と話しかけると、メンションを除去したプロンプトを Sarashina AI に投げて返信します。
- **紐付けチャンネルで質問**:
  `/set_channel` で設定したチャンネル内では、メンションをつけずに普通に発言するだけで自動的に回答します。
