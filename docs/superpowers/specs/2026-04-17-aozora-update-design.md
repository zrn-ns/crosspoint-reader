# 青空文庫EPUB更新機能 設計書

- **Issue**: zrn-ns/crosspoint-jp#45
- **作成日**: 2026-04-17

## Context

青空文庫EPUBダウンロード機能 (`AozoraActivity`) では現在、ダウンロード済み書籍に対して削除しかできない。
青空文庫側でテキストが更新された場合や、ダウンロード時のエラーで破損した書籍を再取得したい
ニーズがある。再ダウンロード（更新）操作を追加する。

同時に、詳細画面のボタン配置を整理する:
- 「ダウンロード」ラベルが長く視認性が低いため「取得」に改称
- 未ダウンロード時はボタン位置を右端（btn4）に移動
- ダウンロード済み時は削除/更新の2操作を左右ボタンに割り当て

## 要件

- **ボタン配置**: `WORK_DETAIL` 状態で以下の4ボタンレイアウトとする
  - 未ダウンロード: `[戻る] [—] [—] [取得]`
  - ダウンロード済み: `[戻る] [—] [削除] [更新]`
- **更新挙動**: 確認ダイアログなしで即座に再ダウンロード開始
- **アトミック更新**: 更新失敗時に旧ファイルを保持（ダウンロード中は `.tmp` に書き、成功時にrename）
- **読書進捗の保持**: 同じファイルパスで更新することで `.crosspoint/` キャッシュの一致を維持

## スコープ外

- 更新確認ダイアログ（UX的に冗長なので採用しない）
- 一覧画面からの一括更新
- 更新日時の記録・表示（将来必要になれば追加）

## 設計

### ボタンハンドラ (`AozoraActivity.cpp:701-742`)

現状は `Button::Confirm` 一つでDL/削除を分岐している。これを物理位置別のハンドラに変更:

```cpp
} else if (state_ == WORK_DETAIL) {
  if (mappedInput.wasPressed(Button::Back)) { popState(); return; }

  const bool alreadyDownloaded = indexManager_.isDownloaded(selectedWorkId_);

  if (!alreadyDownloaded) {
    if (mappedInput.wasPressed(Button::Right)) {
      // 取得 (ダウンロード)
      state_ = DOWNLOADING;
      if (downloadBook()) state_ = WORK_DETAIL;
      else state_ = ERROR;
    }
  } else {
    if (mappedInput.wasPressed(Button::Left)) {
      // 削除
      if (indexManager_.removeEntry(selectedWorkId_)) popState();
    }
    if (mappedInput.wasPressed(Button::Right)) {
      // 更新
      state_ = DOWNLOADING;
      if (updateBook()) state_ = WORK_DETAIL;
      else state_ = ERROR;
    }
  }
}
```

### 新規 `updateBook()` メソッド

```cpp
bool AozoraActivity::updateBook() {
  char url[256];
  snprintf(url, sizeof(url), "%s/api/convert?work_id=%d", API_BASE, selectedWorkId_);

  std::string relPath = AozoraIndexManager::makeRelativePath(
      selectedWorkId_, selectedWorkTitle_, selectedWorkAuthor_);
  char destPath[160];
  char tmpPath[168];
  snprintf(destPath, sizeof(destPath), "%s/%s",
           AozoraIndexManager::AOZORA_DIR, relPath.c_str());
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", destPath);

  // 一時ファイルにダウンロード
  auto result = HttpDownloader::downloadToFile(
      std::string(url), std::string(tmpPath),
      [this](size_t d, size_t t) {
        downloadProgress_ = d;
        downloadTotal_ = t;
        requestUpdate(true);
      },
      30000);

  if (result != HttpDownloader::OK) {
    Storage.remove(tmpPath);  // 一時ファイル破棄、旧ファイルは無傷
    // エラーメッセージ設定
    return false;
  }

  // Atomic swap: 旧ファイル削除 → .tmp を正式名にrename
  Storage.remove(destPath);
  if (!Storage.rename(tmpPath, destPath)) {
    Storage.remove(tmpPath);
    errorMessage_ = "Rename failed";
    return false;
  }

  // indexの更新（タイトル/作者が変わっていない前提なのでentry自体は据え置きで良い）
  return true;
}
```

### ラベル表示 (`AozoraActivity.cpp:1050-1060`)

```cpp
// WORK_DETAIL render
if (alreadyDownloaded) {
  labels = mappedInput.mapLabels(tr(STR_BACK), "", tr(STR_DELETE_CONFIRM), tr(STR_AOZORA_UPDATE));
} else {
  labels = mappedInput.mapLabels(tr(STR_BACK), "", "", tr(STR_AOZORA_GET));
}
GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
```

### i18n

- `STR_AOZORA_GET`: "取得" / "Get"
- `STR_AOZORA_UPDATE`: "更新" / "Update"

既存の `STR_DOWNLOAD` は他所（FontDownload等）で使用中のため保持。

## 変更ファイル

| ファイル | 変更概要 |
|---------|---------|
| `src/activities/settings/AozoraActivity.h` | `updateBook()` メソッド宣言 |
| `src/activities/settings/AozoraActivity.cpp` | WORK_DETAILハンドラ、render、updateBook実装 |
| `lib/I18n/translations/english.yaml` | `STR_AOZORA_GET`, `STR_AOZORA_UPDATE` |
| `lib/I18n/translations/japanese.yaml` | 同上 |

## Done判定基準

- [ ] 未ダウンロード書籍の詳細画面で、btn4ラベルが「取得」と表示される
- [ ] btn4押下でダウンロードが開始される
- [ ] ダウンロード済み書籍の詳細画面で、btn3=「削除」、btn4=「更新」が表示される
- [ ] btn3押下で削除（既存挙動）
- [ ] btn4押下で再ダウンロード、成功後ファイルが更新される
- [ ] 更新失敗時: 旧ファイルが保持され、エラー画面が表示される
- [ ] 更新成功後、読書進捗が保持される（`.crosspoint/` キャッシュ一致）

※ 必須ゲート（ビルド成功・既存機能・差分確認・シークレット）は常に適用

## 検証手順

1. `pio run` でビルド成功確認
2. デバイスにフラッシュ後、青空文庫 → 作品一覧 → 未DL作品を選択
3. btn4（右端）を押下してダウンロードできることを確認
4. ダウンロード完了後、詳細画面のボタンが「削除」「更新」になることを確認
5. btn4（更新）押下で再ダウンロードされ、ファイルが最新化されることを確認
6. 更新前後で読書進捗が保持されていることを確認
7. WiFi OFF状態で更新を試み、旧ファイルが消えずエラー画面になることを確認
