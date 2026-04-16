# ファイル一覧のアーカイブ機能

**Issue**: zrn-ns/crosspoint-jp#44
**日付**: 2026-04-16

## 背景

ファイル一覧でファイルを長押しすると削除のみ可能だが、読了した本などを削除せずに整理したい。アーカイブ（`/Archived/` に移動）を追加する。

## 要件

- 長押しメニューのボタン配置: `[キャンセル][ ][アーカイブ][削除]`
- アーカイブ = ファイル/ディレクトリを `/Archived/` 直下に移動
- 同名ファイルが存在する場合は上書き（先に削除してからrename）
- ディレクトリのアーカイブも対応

## 設計

### ConfirmationActivity の拡張

既存の `neverLabel` 機構（Left ボタンに3つ目の選択肢を表示）を流用する。

**変更**: `backLabel` パラメータを追加。

```cpp
// 変更前:
ConfirmationActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, 
                     const std::string& heading, const std::string& body,
                     const std::string& neverLabel = "", const std::string& confirmLabel = "");

// 変更後:
ConfirmationActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                     const std::string& heading, const std::string& body,
                     const std::string& neverLabel = "", const std::string& confirmLabel = "",
                     const std::string& backLabel = "");
```

`backLabel` が指定された場合、btn1 (Back) にそのラベルを表示。

### ボタン配置とアクション

| ボタン | ラベル | アクション | ActivityResult |
|--------|--------|-----------|---------------|
| Back (btn1) | キャンセル | キャンセル | `isCancelled = true` |
| Left (btn3) | アーカイブ | アーカイブ実行 | `isCancelled = true, data = RESULT_NEVER(2)` |
| Right (btn4) | 削除 | 削除実行 | `isCancelled = false` |

### FileBrowserActivity の変更

長押しハンドラーで ConfirmationActivity を3ボタンモードで起動し、結果に応じて分岐:

```cpp
auto handler = [this, fullPath, isDirectory](const ActivityResult& res) {
  if (!res.isCancelled) {
    // Right ボタン → 削除（既存処理）
  } else if (res.data.has_value()) {
    // Left ボタン → アーカイブ
    archiveFile(fullPath, isDirectory);
  }
  // else: Back ボタン → キャンセル（何もしない）
};
```

### アーカイブ処理 (`archiveFile`)

```
1. Storage.mkdir("/Archived")       // なければ作成
2. destPath = "/Archived/" + filename
3. if (Storage.exists(destPath))    // 同名ファイルがある場合
     Storage.remove(destPath)       //   先に削除（またはremoveDir）
4. Storage.rename(fullPath, destPath) // 移動
5. if (!isDirectory) clearFileMetadata(fullPath)  // EPUBキャッシュクリア
6. loadFiles() + requestUpdate()    // 一覧を更新
```

### i18n

`STR_ARCHIVE` を追加（日本語: 「アーカイブ」、英語: "Archive"）。

## 影響範囲

| ファイル | 変更内容 |
|---------|---------|
| `src/activities/util/ConfirmationActivity.h` | `backLabel` パラメータ追加 |
| `src/activities/util/ConfirmationActivity.cpp` | `backLabel` を mapLabels に反映 |
| `src/activities/home/FileBrowserActivity.cpp` | 3ボタンモード呼び出し + アーカイブ処理追加 |
| `lib/I18n/translations/japanese.yaml` | `STR_ARCHIVE` 追加 |
| `lib/I18n/translations/english.yaml` | `STR_ARCHIVE` 追加 |

## 検証方法

1. `pio run` でビルド成功
2. 実機テスト:
   - ファイル長押しで3ボタンダイアログが表示される
   - アーカイブ選択で `/Archived/` にファイルが移動する
   - 同名ファイルがある場合でもエラーにならない
   - ディレクトリのアーカイブが動作する
   - キャンセル・削除は従来通り動作する
