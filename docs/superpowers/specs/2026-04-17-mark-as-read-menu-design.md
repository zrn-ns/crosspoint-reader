# 長押しメニューに「既読」オプション追加 設計書

- **作成日**: 2026-04-17

## Context

ファイルブラウザで本を長押しすると `ConfirmationActivity` が表示され、現状は
「アーカイブ」「削除」の2択が選べる。既読状態（`ReadingStatus::Finished`）は
既に実装済みで、本を最後まで読むと自動的に設定される。

ユーザーが「最後まで読んでないが既読マークを付けたい」ケース
（試読済・飛ばし読み・別端末で読了済み等）に対応するため、
長押しメニューに「既読」化オプションを追加する。

## 要件

- 長押しメニューに「既読」（英語: "Read"）ボタンを追加
- 押下で即座に該当書籍の `progress.bin` を更新し、`isFinished=1` にする
- 未読状態（`progress.bin` 不在）でも動作する（新規作成）
- 既に既読の書籍に実行しても副作用なし（isFinished上書き）
- EPUB / XTC 両形式で動作
- 処理完了後、ファイル一覧のアイコンが `BookFinished` に更新される

## スコープ外

- 「未読に戻す」操作（将来別Issueで対応）
- RecentBooksActivity への同様の長押しメニュー追加
- 読書進捗の巻き戻し（既読化しても spineIndex/currentPage は変更しない）

## 設計

### 1. ConfirmationActivity の3択対応

現状の `ConfirmationActivity` は Back/Left/Right の3ボタンを使い、`Button::Confirm` (btn2) は未使用。これを拡張して中央ボタンの第3選択肢を追加する。

**変更点** (`ConfirmationActivity.h/.cpp`):

- コンストラクタ末尾に `confirmMiddleLabel = ""` 引数を追加（デフォルト空で後方互換）
- `RESULT_MIDDLE = 3` 定数を追加
- `loop()` で `confirmMiddleLabel` が非空のとき `Button::Confirm` を処理し、`MenuResult{RESULT_MIDDLE}` を返す
- `render()` で `confirmMiddleLabel` が非空ならそれを btn2 のラベルに渡す

### 2. FileBrowserActivity のメニュー拡張

`FileBrowserActivity.cpp:300-303` で `ConfirmationActivity` 生成時に第4引数として既読ラベルを追加:

```cpp
startActivityForResult(
    std::make_unique<ConfirmationActivity>(
        renderer, mappedInput, heading, "",
        tr(STR_ARCHIVE), tr(STR_DELETE), tr(STR_CANCEL),
        tr(STR_MARK_AS_READ)),  // NEW: btn2 = 既読
    handler);
```

ハンドラ内の分岐を以下のように整理:

```cpp
if (res.isCancelled && std::holds_alternative<MenuResult>(res.data)) {
  const int code = std::get<MenuResult>(res.data).action;
  if (code == ConfirmationActivity::RESULT_NEVER) {
    // アーカイブ
  } else if (code == ConfirmationActivity::RESULT_MIDDLE) {
    // 既読にする
    ReadingStatusHelper::markAsFinished(fullPath);
  } else {
    // キャンセル
  }
} else if (!res.isCancelled) {
  // 削除
}
```

### 3. ReadingStatusHelper::markAsFinished の新規実装

`ReadingStatusHelper.h/.cpp` に静的メソッドを追加:

```cpp
// 指定ファイルを既読（isFinished=1）にマーク。
// progress.bin が無ければ新規作成。EPUB/XTC の形式を自動判定。
static bool markAsFinished(const std::string& filePath);
```

**実装方針**:
1. ファイル拡張子で EPUB / XTC を判定
2. `std::hash<std::string>{}(filePath)` でハッシュ化、`.crosspoint/{epub|xtc}_<hash>/progress.bin` のパスを構築
3. ディレクトリが無ければ作成
4. 既存の `progress.bin` があれば末尾バイトだけ書き換え、無ければ以下を書き込み:
   - EPUB: `[spineIndex=0 (2B)][currentPage=0 (2B)][pageCount=0 (2B)][isFinished=1 (1B)]`
   - XTC: `[currentPage=0 (4B)][isFinished=1 (1B)]`

### 4. i18n

- `STR_MARK_AS_READ`: "Read" / "既読"

## 変更ファイル

| ファイル | 変更概要 |
|---------|---------|
| `src/activities/util/ConfirmationActivity.h` | `confirmMiddleLabel` 引数、`RESULT_MIDDLE` 定数 |
| `src/activities/util/ConfirmationActivity.cpp` | Confirm ボタン処理、btn2 ラベル表示 |
| `src/activities/home/FileBrowserActivity.cpp` | メニュー拡張、ハンドラ分岐、`markAsFinished` 呼出 |
| `src/ReadingStatusHelper.h` | `markAsFinished` 宣言 |
| `src/ReadingStatusHelper.cpp` | `markAsFinished` 実装 |
| `lib/I18n/translations/english.yaml` | `STR_MARK_AS_READ: "Read"` |
| `lib/I18n/translations/japanese.yaml` | `STR_MARK_AS_READ: "既読"` |

## Done 判定基準

- [ ] 長押しメニュー表示時、btn2 に「既読」が表示される
- [ ] btn2 押下で `progress.bin` が更新され、アイコンが `BookFinished` に変わる
- [ ] 未読書籍（`progress.bin` 不在）でも既読化できる
- [ ] 既に既読の書籍に実行しても副作用がない
- [ ] 既存のアーカイブ/削除/キャンセル動作に影響なし

※ 必須ゲート（ビルド成功・既存機能・差分確認・シークレット）は常に適用

## 検証手順

1. `pio run` でビルド成功確認
2. フラッシュ後、SDカードにEPUB書籍を配置
3. ファイルブラウザで対象書籍を長押し → メニューに「既読」が表示されることを確認
4. btn2 押下 → アイコンが `BookFinished` (黒塗り+チェック) に変わることを確認
5. XTC ファイルでも同様の動作を確認
6. 既存動作（アーカイブ/削除）が引き続き動作することを確認
