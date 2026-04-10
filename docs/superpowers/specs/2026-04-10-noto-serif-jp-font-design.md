# Noto Serif JP フォント追加 設計書

**Issue**: zrn-ns/crosspoint-jp#38
**日付**: 2026-04-10

## 概要

ダウンロード可能フォントに日本語明朝体「Noto Serif JP」を追加する。
現状、日本語フォントはゴシック体（NotoSansJp, BIZUDGothic）のみで、明朝体がない。

## 設計

### フォント仕様

| 項目 | 値 |
|------|-----|
| ファミリー名 | `NotoSerifJp` |
| 説明文 | `"Serif Japanese (JIS X 0213)"` |
| ソースフォント | NotoSerifJP-VF.otf（可変フォント、Subset版） |
| ソースURL | `https://github.com/notofonts/noto-cjk/raw/main/Serif/Variable/OTF/Subset/NotoSerifJP-VF.otf` |
| スタイル | Regular (wght: 400), Bold (wght: 700) |
| サイズ | 10, 12, 14, 16, 18 pt |
| Unicode範囲 | `ascii,latin1,punctuation,cjk` |
| コードポイントファイル | `codepoints/japanese_jis0213.txt`（既存共用） |

### sd-fonts.yaml への追記内容

```yaml
  - name: NotoSerifJp
    description: "Serif Japanese (JIS X 0213)"
    intervals: ascii,latin1,punctuation,cjk
    codepoints_file: codepoints/japanese_jis0213.txt
    sizes: [10, 12, 14, 16, 18]
    styles:
      regular: {url: "https://github.com/notofonts/noto-cjk/raw/main/Serif/Variable/OTF/Subset/NotoSerifJP-VF.otf", variable: {wght: 400}}
      bold:    {url: "https://github.com/notofonts/noto-cjk/raw/main/Serif/Variable/OTF/Subset/NotoSerifJP-VF.otf", variable: {wght: 700}}
```

Japanese セクション内、既存の `BIZUDGothic` の後に配置する。

### 変更対象ファイル

1. **`lib/EpdFont/scripts/sd-fonts.yaml`** — フォントファミリー定義を追加
2. **`docs/fonts.json`** — ビルド後に `generate-font-manifest.py` で自動再生成

### 変更不要なもの

- ファームウェアコード — フォントダウンロード機能は汎用的に実装済み
- コードポイントファイル — 既存の `japanese_jis0213.txt` を共用
- ビルドスクリプト — 可変フォント対応済み（`fonttools.instancer` で静的インスタンス抽出）
- GitHub Actionsワークフロー — `sd-fonts.yaml` 変更でトリガーされる既存の `release-fonts.yml` がそのまま動作

### リリースフロー

1. `sd-fonts.yaml` を変更してmasterにpush
2. GitHub Actions (`release-fonts.yml`) が自動実行:
   - 可変フォントをダウンロード → `fonttools.instancer` でRegular/Bold静的インスタンス抽出
   - `fontconvert_sdcard.py` で各サイズの `.cpfont` ファイル生成
   - `generate-font-manifest.py` で `fonts.json` 再生成
   - 全ファイルを `sd-fonts` リリースタグにアップロード
3. デバイス側は次回「設定 → フォントダウンロード」で新フォントが表示される

### 予想される出力サイズ

NotoSansJpと同程度（JIS X 0213で約14,000文字、2ビットグレースケール）:

| サイズ | 予想容量 |
|--------|----------|
| 10pt | ~2.5 MB |
| 12pt | ~6 MB |
| 14pt | ~8 MB |
| 16pt | ~10 MB |
| 18pt | ~13 MB |
| **合計** | **~40 MB** |

## Done 判定基準

- [ ] `sd-fonts.yaml` にNotoSerifJpの設定が正しく追加されている
- [ ] `pio run` でファームウェアビルドが通る（コード変更なしのため形式的確認）
- [ ] GitHub Actionsでフォントビルドが成功する
- [ ] デバイスのフォントダウンロード画面にNotoSerifJpが表示される

※ 必須ゲート（動作検証・既存機能・差分確認・シークレット）は常に適用
