# Noto Serif JP フォント追加 実装計画

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ダウンロード可能フォントに日本語明朝体 NotoSerifJp を追加する

**Architecture:** `sd-fonts.yaml` に1エントリ追加するのみ。ビルドスクリプト・ファームウェアコード・GitHub Actionsワークフローの変更は不要。CIでのフォントビルド＆リリースは既存パイプラインがそのまま動作する。

**Tech Stack:** YAML設定、Python フォントビルドパイプライン（既存）、GitHub Actions（既存）

---

### Task 1: sd-fonts.yaml に NotoSerifJp エントリを追加

**Files:**
- Modify: `lib/EpdFont/scripts/sd-fonts.yaml:220-221`（BIZUDGothic定義の直後に追記）

- [ ] **Step 1: sd-fonts.yaml を編集**

`lib/EpdFont/scripts/sd-fonts.yaml` の BIZUDGothic エントリ（220行目）の直後、`# ── Additional scripts` コメント（222行目）の直前に、以下のブロックを追加する:

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

追加後のJapaneseセクション全体は以下のようになる:

```yaml
  # ── Japanese ─────────────────────────────────────────────────────────

  - name: NotoSansJp
    description: "Sans-serif Japanese (JIS X 0213)"
    intervals: ascii,latin1,punctuation,cjk
    codepoints_file: codepoints/japanese_jis0213.txt
    sizes: [10, 12, 14, 16, 18]
    styles:
      regular: {url: "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/Japanese/NotoSansCJKjp-Regular.otf"}
      bold:    {url: "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/Japanese/NotoSansCJKjp-Bold.otf"}

  - name: BIZUDGothic
    description: "UD Gothic Japanese (MORISAWA, optimized for readability)"
    intervals: ascii,latin1,punctuation,cjk
    codepoints_file: codepoints/japanese_jis0213.txt
    sizes: [10, 12, 14, 16, 18]
    styles:
      regular: {url: "https://github.com/googlefonts/morisawa-biz-ud-gothic/raw/main/fonts/ttf/BIZUDGothic-Regular.ttf"}
      bold:    {url: "https://github.com/googlefonts/morisawa-biz-ud-gothic/raw/main/fonts/ttf/BIZUDGothic-Bold.ttf"}

  - name: NotoSerifJp
    description: "Serif Japanese (JIS X 0213)"
    intervals: ascii,latin1,punctuation,cjk
    codepoints_file: codepoints/japanese_jis0213.txt
    sizes: [10, 12, 14, 16, 18]
    styles:
      regular: {url: "https://github.com/notofonts/noto-cjk/raw/main/Serif/Variable/OTF/Subset/NotoSerifJP-VF.otf", variable: {wght: 400}}
      bold:    {url: "https://github.com/notofonts/noto-cjk/raw/main/Serif/Variable/OTF/Subset/NotoSerifJP-VF.otf", variable: {wght: 700}}

  # ── Additional scripts (no complex shaping needed) ─────────────────────
```

- [ ] **Step 2: YAML構文を検証**

Run: `python3 -c "import yaml; yaml.safe_load(open('lib/EpdFont/scripts/sd-fonts.yaml')); print('YAML OK')"`
Expected: `YAML OK`

- [ ] **Step 3: 差分を確認**

Run: `git diff lib/EpdFont/scripts/sd-fonts.yaml`
Expected: NotoSerifJp ブロック（9行）の追加のみが表示される。他の行に変更がないことを確認。

- [ ] **Step 4: ソースURLの到達性を確認**

Run: `curl -sI -o /dev/null -w '%{http_code}' -L "https://github.com/notofonts/noto-cjk/raw/main/Serif/Variable/OTF/Subset/NotoSerifJP-VF.otf"`
Expected: `200`（HTTP 200 OK。302リダイレクト経由で最終的に200になる）

- [ ] **Step 5: コミット**

```bash
git add lib/EpdFont/scripts/sd-fonts.yaml
git commit -m "✨ ダウンロード可能フォントにNoto Serif JP（明朝体）を追加"
```

### Task 2: ファームウェアビルド確認（形式的検証）

**Files:** なし（コード変更なし。sd-fonts.yamlはビルド対象外）

- [ ] **Step 1: ファームウェアビルドが通ることを確認**

Run: `pio run 2>&1 | tail -5`
Expected: `SUCCESS` が表示される。sd-fonts.yaml はファームウェアビルドに影響しないため、既存ビルドがそのまま通る。

### Task 3: GitHub Actionsでのフォントビルド確認（push後）

**Files:** なし

- [ ] **Step 1: masterにpush**

Run: `git push origin master`

- [ ] **Step 2: GitHub Actionsの実行を確認**

Run: `gh run list --workflow=release-fonts.yml --limit=1`
Expected: `release-fonts.yml` ワークフローが `in_progress` または `completed` で表示される。

- [ ] **Step 3: ワークフローの成功を確認**

Run: `gh run watch` （最新のランを監視）
Expected: 全ステップが緑（成功）。NotoSerifJpの `.cpfont` ファイルが `sd-fonts` リリースにアップロードされる。

- [ ] **Step 4: リリースアセットにNotoSerifJpファイルが存在することを確認**

Run: `gh release view sd-fonts --json assets -q '.assets[].name' | grep NotoSerifJp`
Expected:
```
NotoSerifJp_10.cpfont
NotoSerifJp_12.cpfont
NotoSerifJp_14.cpfont
NotoSerifJp_16.cpfont
NotoSerifJp_18.cpfont
```

- [ ] **Step 5: マニフェストにNotoSerifJpが含まれることを確認**

Run: `gh release download sd-fonts --pattern 'fonts.json' --output /tmp/fonts.json && python3 -c "import json; d=json.load(open('/tmp/fonts.json')); print([f['name'] for f in d['families'] if 'Serif' in f['name'] and 'Jp' in f['name']])"`
Expected: `['NotoSerifJp']`
