# Dev Build 自動デプロイ 実装プラン

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** masterへのpush時に自動でファームウェアをビルドし、GitHub Releaseとして配布する

**Architecture:** 既存の `release.yml` のビルドステップを流用し、タイムスタンプベースのタグでprereleaseを作成する単一ワークフロー

**Tech Stack:** GitHub Actions, PlatformIO, softprops/action-gh-release@v2

**Spec:** `docs/superpowers/specs/2026-04-06-dev-build-deploy-design.md`

---

## Done 判定基準

- [ ] `dev-build.yml` がmaster pushでトリガーされる
- [ ] `gh_release` 環境でビルドが成功する
- [ ] firmware.bin, bootloader.bin, partitions.bin の3ファイルがリリースアセットとしてアップロードされる
- [ ] リリースが prerelease フラグ付きで作成される
- [ ] `make_latest: false` により正式リリースの Latest バッジに影響しない
- [ ] リリースbodyにコミットSHA・メッセージが含まれる
- [ ] 既存ワークフロー（ci.yml, release.yml）に影響しない
※ 必須ゲート（動作検証・既存機能・差分確認・シークレット）は常に適用

---

### Task 1: dev-build.yml ワークフロー作成

**Files:**
- Create: `.github/workflows/dev-build.yml`

- [ ] **Step 1: ワークフローファイルを作成**

```yaml
name: Dev Build

on:
  push:
    branches: [master]

permissions:
  contents: write

jobs:
  dev-build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v6
        with:
          submodules: recursive

      - uses: actions/setup-python@v6
        with:
          python-version: '3.14'

      - name: Install uv
        uses: astral-sh/setup-uv@v7
        with:
          version: "latest"
          enable-cache: false

      - name: Install PlatformIO Core
        run: uv pip install --system -U https://github.com/pioarduino/platformio-core/archive/refs/tags/v6.1.19.zip

      - name: Build CrossPoint
        run: |
          set -euo pipefail
          pio run -e gh_release

          firmware="$(find .pio/build -maxdepth 3 -type f -path "*/firmware.bin" | grep "/gh_release/" | head -n1)"
          bootloader="$(find .pio/build -maxdepth 3 -type f -path "*/bootloader.bin" | grep "/gh_release/" | head -n1)"
          partitions="$(find .pio/build -maxdepth 3 -type f -path "*/partitions.bin" | grep "/gh_release/" | head -n1)"

          if [ -z "${firmware}" ] || [ -z "${bootloader}" ] || [ -z "${partitions}" ]; then
            echo "Expected gh_release artifacts not found."
            find .pio/build -maxdepth 4 -type f | sort || true
            exit 1
          fi

          cp "${firmware}" firmware.bin
          cp "${bootloader}" bootloader.bin
          cp "${partitions}" partitions.bin

      - name: Generate release metadata
        id: meta
        run: |
          TIMESTAMP="$(date -u +%Y%m%d-%H%M%S)"
          DISPLAY_TIME="$(date -u +%Y-%m-%d\ %H:%M:%S)"
          SHORT_SHA="${GITHUB_SHA::7}"
          COMMIT_MSG="$(git log -1 --pretty=%s)"

          echo "tag_name=dev-${TIMESTAMP}" >> "$GITHUB_OUTPUT"
          echo "release_name=Dev Build ${DISPLAY_TIME}" >> "$GITHUB_OUTPUT"
          echo "short_sha=${SHORT_SHA}" >> "$GITHUB_OUTPUT"
          echo "commit_msg=${COMMIT_MSG}" >> "$GITHUB_OUTPUT"

      - name: Create Release
        uses: softprops/action-gh-release@v2
        with:
          tag_name: ${{ steps.meta.outputs.tag_name }}
          name: ${{ steps.meta.outputs.release_name }}
          prerelease: true
          make_latest: false
          files: |
            firmware.bin
            bootloader.bin
            partitions.bin
          body: |
            ## Dev Build

            | 項目 | 値 |
            |------|-----|
            | コミット | [`${{ steps.meta.outputs.short_sha }}`](${{ github.server_url }}/${{ github.repository }}/commit/${{ github.sha }}) |
            | メッセージ | ${{ steps.meta.outputs.commit_msg }} |
            | ビルド環境 | `gh_release` |

            ### 📦 アセット

            | ファイル | 用途 |
            |---------|------|
            | `firmware.bin` | OTAアップデート / フラッシュ書き込み |
            | `bootloader.bin` | 初回書き込み / 復旧用 |
            | `partitions.bin` | 初回書き込み / 復旧用 |
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

- [ ] **Step 2: 差分を確認**

```bash
git diff
cat .github/workflows/dev-build.yml
```

ワークフローの構造が以下を満たすことを確認:
- トリガー: `push` → `branches: [master]`
- permissions: `contents: write`
- ビルドステップが `release.yml` と一致
- `tag_name` で直接タグ指定（git pushでタグを作らない）
- `prerelease: true`, `make_latest: false`
- 3ファイルすべてが `files:` に含まれる

- [ ] **Step 3: コミット**

```bash
git add .github/workflows/dev-build.yml
git commit -m "✨ masterへのpush時にdev buildを自動リリースするワークフローを追加"
```

---

### Task 2: 動作検証（masterへpush）

- [ ] **Step 1: masterにpushしてワークフローをトリガー**

```bash
git push origin master
```

- [ ] **Step 2: GitHub Actionsでワークフロー実行を確認**

```bash
gh run list --workflow=dev-build.yml --limit 1
```

期待値: `Dev Build` ワークフローが `in_progress` または `completed` で表示される

- [ ] **Step 3: 既存CIも正常に動作していることを確認**

```bash
gh run list --workflow=ci.yml --limit 1
```

期待値: `CI (build)` ワークフローも同時にトリガーされ、正常に動作

- [ ] **Step 4: リリースが作成されたことを確認**

```bash
gh release list --limit 3
```

期待値:
- `Dev Build YYYY-MM-DD HH:MM:SS` がprerelease付きで表示
- 既存の正式リリースの Latest バッジに影響なし

- [ ] **Step 5: リリースアセットを確認**

```bash
gh release view "$(gh release list --limit 1 --json tagName -q '.[0].tagName')" --json assets -q '.assets[].name'
```

期待値: `firmware.bin`, `bootloader.bin`, `partitions.bin` の3ファイルが表示
