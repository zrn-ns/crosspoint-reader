# CrossPoint JP プロジェクト名変更・README刷新 設計

## 概要

フォークの独自性を明確にするため、プロジェクト名を「CrossPoint JP」に変更し、
日本語ユーザー向けのREADMEに刷新する。

対応Issue: https://github.com/zrn-ns/crosspoint-reader/issues/1

## 変更1: リポジトリ名変更

- `zrn-ns/crosspoint-reader` → `zrn-ns/crosspoint-jp`
- `gh repo rename crosspoint-jp` で変更
- 旧URLはGitHubが自動リダイレクト
- ローカルのgit remote originも新URLに更新

## 変更2: README.md 刷新

日本語で全面書き直し。以下の構成：

### 構成

1. **タイトル・概要**
   - CrossPoint JP とは何か
   - 日本語EPUBの閲覧に特化したXteink向けファームウェアフォーク
   - 本家 crosspoint-reader をベースに、CJKフォークの改善を統合

2. **注意事項**
   - 動作保証なし。自己責任で使用すること
   - 現状 Xteink X3 でのみ動作確認済み。X4 での動作は未確認
   - 公式 CrossPoint に CJK サポートが入った場合はお役御免の予定

3. **主な機能・改善点**
   - 縦書き表示対応
   - テーブル表示対応
   - SDカードフォント（BIZ UDGothic、NotoSansJP等の日本語フォント）
   - 日本語向けレンダリング品質の向上（CJK文字間隔、段落インデント等）
   - 行間隔の縦書き/横書き別設定
   - 各種バグ修正

4. **インストール方法**
   - リリースページから firmware.bin をダウンロード
   - USB接続でフラッシュ書き込み

5. **おすすめEPUB入手先**
   - 青空文庫 EPUB変換: https://aozora.orihasam.com/#index.html

6. **ベースとなったプロジェクト**
   - crosspoint-reader/crosspoint-reader (本家 v1.2.0)
   - aBER0724/crosspoint-reader-cjk (CJKフォーク)
   - PR #1392 (SDカードフォント)
   - PR #875 (X3サポート)

7. **ライセンス**
   - MIT License（既存LICENSEへのリンク）

## 変更3: 不要ドキュメント削除

- `README-JA.md` — 日本語READMEがメインになるため不要
- `GOVERNANCE.md` — CJKフォークのコミュニティガバナンス（個人フォークには不要）
- `SCOPE.md` — CJKフォークのスコープ定義（新READMEで代替）
- `AGENTS.md` — CLAUDE.mdで代替済み

残す: `USER_GUIDE.md`, `LICENSE`

## 変更4: リポジトリ内のURL参照更新

リネーム後にリダイレクトは効くが、明示的に新URLへ更新するファイル：
- `src/activities/settings/FontDownloadActivity.h` — FONT_MANIFEST_URL
- `.github/workflows/dev-build.yml` — リポジトリ参照（`${{ github.repository }}` で自動解決のため不要）
- `CLAUDE.md` — リポジトリ名への言及箇所

## 変更5: LICENSE の著作権表示追加

現在のLICENSEにはCJKフォーク（aBER）の著作権のみ記載。
本家（Dave Allie）の著作権表示も追加し、MITライセンスの条件を満たす。

```
Copyright (c) 2025 Dave Allie
Copyright (c) 2025 aBER
```

## 変更しないもの

- コード内のクラス名（`CrossPointSettings` 等）
- キャッシュディレクトリ（`.crosspoint/`）
- `platformio.ini` の `[crosspoint]` セクション
- ワークフロー内の `${{ github.repository }}` 参照（自動で新名に解決）
