# NotoSansCJK Bold スタイル追加設計

## 概要

EPUB見出し（h1〜h6）がBoldで描画されない問題を解消する。
原因はSDカードフォント（NotoSansCJK）にBoldスタイルの `.cpfont` が含まれていないこと。

## 背景

- ChapterHtmlSlimParser は見出し要素に自動で BOLD フラグを設定している（`boldUntilDepth` メカニズム）
- SdCardFont / GfxRenderer は既にマルチスタイル対応済み（MAX_STYLES=4）
- しかし `sd-fonts.yaml` の NotoSansCJK 定義が Regular のみのため、Bold要求時に Regular にフォールバックしている

## 変更内容

### 1. `lib/EpdFont/scripts/sd-fonts.yaml`

NotoSansCJK に Bold スタイルを追加:

```yaml
- name: NotoSansCJK
  description: "Sans-serif (Chinese, Japanese, Korean)"
  intervals: ascii,latin1,punctuation,cjk
  sizes: [12, 14, 16, 18]
  styles:
    regular: {url: "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf"}
    bold: {url: "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Bold.otf"}
```

### 2. `src/activities/settings/FontDownloadActivity.h`

`FONT_MANIFEST_URL` を自分のフォーク (`zrn-ns/crosspoint-reader`) のリリースURLに変更:

```cpp
#define FONT_MANIFEST_URL "https://github.com/zrn-ns/crosspoint-reader/releases/download/sd-fonts/fonts.json"
```

### 3. GitHub Actions でフォントリリース

`.github/workflows/release-fonts.yml` を自分のフォークで実行し、Bold付き `.cpfont` と `fonts.json` を配布する。

## コード変更不要な部分

- **SdCardFont**: 既にマルチスタイル対応（`PerStyle styles_[MAX_STYLES]`）
- **GfxRenderer**: 既にBoldフォント選択・フォールバック実装済み
- **ChapterHtmlSlimParser**: 既に見出しにBOLDフラグ設定済み
- **FontDownloadActivity**: マニフェストの styles 配列を読み取り済み

## RAM影響

- load()直後: +6.5 KB（intervals等の常駐データ）
- prewarm時（見出し50〜100文字）: +5〜10 KB
- 380KB RAM制約に対して問題なし

## 配布サイズ影響

- NotoSansCJK の各サイズ（12, 14, 16, 18pt）に Bold 分が追加
- 1サイズあたり数MBの増加（SDカードダウンロード時間は増えるが、ストレージは十分）

## Done判定基準

- [ ] `sd-fonts.yaml` に NotoSansCJK Bold が追加されている
- [ ] `FONT_MANIFEST_URL` が `zrn-ns/crosspoint-reader` を指している
- [ ] `pio run` でビルドが通る
- [ ] GitHub Actions でフォントリリースが成功し、`fonts.json` に Bold スタイルが含まれる
※ 必須ゲート（動作検証・既存機能・差分確認・シークレット）は常に適用
