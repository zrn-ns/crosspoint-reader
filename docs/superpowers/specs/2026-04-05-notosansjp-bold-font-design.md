# NotoSansJp Bold フォント追加 & CJK/Hangulフォント整理

## 概要

SDカードフォント `NotoSansJpRegularOnly` はRegularスタイルのみを含むため、
Bold要求時にFaux Bold（1pxずらし2パス描画）が使用され、読みづらい。

本プロジェクトではChinese/Koreanのサポートは不要のため、
CJK/Hangul系フォントを整理し、日本語に特化した `NotoSansJp` に統合する。

## 背景

- Issue: https://github.com/zrn-ns/crosspoint-reader/issues/2
- Faux Bold は `GfxRenderer.cpp:2170-2234` で実装されている
- 1bit E-Inkディスプレイでは描画品質改善に本質的な限界がある
- デバイス側ファームウェアは既に `.cpfont` v4 マルチスタイルに対応済み

## 変更内容

### sd-fonts.yaml の変更

1. **削除**: `NotoSansCJK`（簡体中国語ベース、不要）
2. **削除**: `NotoSerifCJK`（簡体中国語ベース、不要）
3. **削除**: `NotoSansHangul`（韓国語、不要）
4. **削除**: `NotoSansJpRegularOnly`（NotoSansJpに統合）
5. **追加**: `NotoSansJp`（Regular + Bold, JIS X 0213, 4サイズ）

```yaml
- name: NotoSansJp
  description: "Sans-serif Japanese (JIS X 0213)"
  intervals: ascii,latin1,punctuation,cjk
  codepoints_file: codepoints/japanese_jis0213.txt
  sizes: [12, 14, 16, 18]
  styles:
    regular: {url: "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/Japanese/NotoSansCJKjp-Regular.otf"}
    bold:    {url: "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/Japanese/NotoSansCJKjp-Bold.otf"}
```

## 影響範囲

- **ファームウェア変更**: なし
- **フォントビルドパイプライン**: `build-sd-fonts.py` が自動的に Regular+Bold 複合 `.cpfont` を生成
- **マニフェスト**: `generate-font-manifest.py` が `fonts.json` に自動追加
- **CI/CD**: `release-fonts.yml` ワークフロー実行で GitHub Release に自動デプロイ

## ダウンロードサイズ

| フォント | 推定サイズ |
|---------|-----------|
| NotoSansJp (4サイズ, Regular + Bold, JIS X 0213) | ~6 MB |

## Done 判定基準

- [ ] `sd-fonts.yaml` から `NotoSansCJK`, `NotoSerifCJK`, `NotoSansHangul`, `NotoSansJpRegularOnly` が削除されている
- [ ] `sd-fonts.yaml` に `NotoSansJp`（Regular + Bold, 4サイズ, JIS X 0213）が追加されている
- [ ] `pio run` でファームウェアビルドが通る（ファームウェア変更なしの確認）
- ※ フォントビルド・リリースは別途 CI/CD で実行
- ※ 必須ゲート（動作検証・既存機能・差分確認・シークレット）は常に適用
