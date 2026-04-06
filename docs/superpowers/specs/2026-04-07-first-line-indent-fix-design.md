# 一行目インデント設定の不具合修正 設計

## 概要

「一行目インデント」設定の3つの不具合を修正する。

対応Issue: https://github.com/zrn-ns/crosspoint-jp/issues/21

## 修正1: インデント幅を全角1文字分に変更

### 現状
`ParsedText.cpp:175` でインデント幅を `cjkCharWidth * 2`（全角2文字分）で計算している。
フォールバックは `spaceWidth * 6`。

### 変更
- `cjkCharWidth * 2` → `cjkCharWidth`（全角1文字分）
- フォールバック: `spaceWidth * 6` → `spaceWidth * 3`

### 対象ファイル
- `lib/Epub/Epub/ParsedText.cpp` 175行目付近

## 修正2: 縦書きレイアウトにインデント対応を追加

### 現状
`ParsedText::layoutVerticalColumns()` にはインデント処理が一切存在しない。
横書き用の `layoutAndExtractLines()` にのみインデントロジックがある。

### 変更
`layoutVerticalColumns()` の先頭付近で、横書きと同じ条件でインデント幅を計算し、
各段落の最初の列（isFirstLine相当）の `colYpos` にインデント分のオフセットを加算する。

条件（横書きと同一）:
- `firstLineIndent` が有効
- CSS `text-indent` が未定義（`blockStyle.textIndent == 0 && !blockStyle.textIndentDefined`）
- テキスト配置が `Justify` または `Left`

縦書きでの「一行目インデント」は、最初の列の先頭（上端）にスペースを空けることに相当する。

### 対象ファイル
- `lib/Epub/Epub/ParsedText.cpp` `layoutVerticalColumns()` メソッド内

## 修正3: デフォルト値を有効に変更

### 現状
`CrossPointSettings.h` で `firstLineIndent` の初期値が `0`（無効）。

### 変更
初期値を `1`（有効）に変更。

### 対象ファイル
- `src/CrossPointSettings.h`

## 影響範囲

- `firstLineIndent` は既にセクションキャッシュの検証パラメータに含まれているため、
  設定変更時にキャッシュが自動的に無効化される。追加対応不要。
- 英語テキストでは全角1文字分のインデントが不自然に見える可能性があるが、
  Issue記載の通り「どうしようもない」ため対応しない。
