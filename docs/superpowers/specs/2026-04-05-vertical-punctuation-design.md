# OpenType `vert`フィーチャーによる縦書きグリフ対応 設計

**Date**: 2026-04-05
**Status**: Approved
**Parent**: 縦書き表示サポート（Issue #15）

## 目的

縦書き表示時の句読点・括弧・長音記号等を、フォント本来の縦書き用代替グリフで描画する。
OpenType GSUBテーブルの`vert`フィーチャーから代替グリフを抽出し、`.cpfont`に格納して
レンダラーから参照可能にする。

## スコープ

### 含むもの
- `fontconvert.py`で OpenType `vert`マッピングを抽出し`.cpfont`に格納
- `.cpfont`フォーマット拡張（vertセクション追加、バージョンバンプ）
- `SdCardFont`のvert グリフ読み込み・プリウォーム・取得
- `drawTextVertical()`でvertグリフを優先使用
- ビルトインフォント（`.h`ファイル）への対応

### 含まないもの
- `vert`以外のOpenType GSUB フィーチャー（`vrt2`等は将来検討）
- ExternalFont（.bin形式）のvert対応
- フォントマニフェスト・ダウンロードシステムの変更（既存フォントは再生成で対応）

## 制約

- ESP32-C3: 380KB RAM。vertグリフは対象codepointのみ格納し、メモリ消費を最小化
- 既存の横書きパスに影響を与えない
- `.cpfont` v4との後方互換: v4ファイルは`hasVert=false`として読める（vertセクションなし）

## 設計

### 1. fontconvert.py の改修

FreeTypeでフォントを読み込んだ後、`fontTools`ライブラリを使用してOpenType GSUBテーブルから
`vert`フィーチャーのSingleSubstitution lookupを抽出する。

```python
from fontTools.ttLib import TTFont

def extract_vert_mappings(font_path):
    """Extract codepoint -> substitute glyph ID mapping from 'vert' feature."""
    tt = TTFont(font_path)
    if 'GSUB' not in tt:
        return {}
    gsub = tt['GSUB'].table
    cmap = tt.getBestCmap()
    reverse_cmap = {v: k for k, v in cmap.items()}  # glyphName -> codepoint

    vert_glyph_names = {}  # original glyphName -> substitute glyphName
    for feature in gsub.FeatureList.FeatureRecord:
        if feature.FeatureTag == 'vert':
            for lookup_idx in feature.Feature.LookupListIndex:
                lookup = gsub.LookupList.Lookup[lookup_idx]
                for subtable in lookup.SubTable:
                    if hasattr(subtable, 'mapping'):
                        vert_glyph_names.update(subtable.mapping)

    # Convert to codepoint -> substitute glyphName
    result = {}
    for orig_name, sub_name in vert_glyph_names.items():
        if orig_name in reverse_cmap:
            cp = reverse_cmap[orig_name]
            result[cp] = sub_name
    return result
```

- 各`vert`対象のcodepoint について、代替グリフのビットマップ・メトリクス（EpdGlyph）を生成
- 通常グリフと同じ`--2bit`/`--compress`設定を適用
- `.cpfont`ファイルの各スタイルセクションの末尾に「vertセクション」を追加

### 2. .cpfontフォーマット拡張

#### ヘッダー変更
- `CPFONT_VERSION`: 4 → 5
- フラグフィールド（16bit）: bit0=is2Bit（既存）、**bit1=hasVert**（新規）

#### スタイルTOC（各スタイルの32バイトヘッダー）変更
- 末尾に `uint32_t vertSectionOffset` を追加
- vertSectionOffset == 0 の場合、そのスタイルにはvertデータなし

#### vertセクション構造（スタイルごと）
```
uint16_t vertCount          // 縦書き代替グリフ数
uint32_t codepoints[vertCount]  // 対象codepoint（ソート済み、二分探索用）
EpdGlyph glyphs[vertCount]     // 代替グリフメトリクス
uint8_t  bitmaps[...]          // 連結されたビットマップデータ
```

- codepointはソート済みで格納し、O(log n)の二分探索で検索可能
- 一般的なCJKフォントで`vert`対象は50-200グリフ程度
- メモリ見積もり: 100グリフ × (4+14)バイト = 1.8KB（メトリクスのみ、ビットマップは必要時にSD読み込み）

### 3. SdCardFont の読み込み

#### ファイルオープン時
- `hasVert`フラグを確認し、各スタイルの`vertSectionOffset`を記録
- vertデータ自体はこの時点では読み込まない（遅延読み込み）

#### prewarm() 拡張
- 縦書きモード時、prewarmの対象codepointリストに対してvertマッピングを確認
- vert対象のcodepointについて、代替グリフのEpdGlyph + ビットマップをSD から読み込み
- `miniVertGlyphs[]` / `miniVertBitmap` として既存のminiGlyphs方式と同じパターンで保持

#### 新メソッド
```cpp
const EpdGlyph* getVertGlyph(uint32_t codepoint, EpdFontFamily::Style style) const;
const uint8_t* getVertBitmap(uint32_t codepoint, EpdFontFamily::Style style) const;
```
- vertセクションのcodepoint配列を二分探索
- ヒットすればvert用EpdGlyph/bitmapを返す
- ミスすればnullptr（呼び出し側は通常グリフにフォールバック）

### 4. drawTextVertical() の変更

```cpp
// 現在: 全文字を通常グリフで描画
drawText(effectiveFontId, x, yPos, charBuf, black, style);

// 変更後:
const EpdGlyph* vertGlyph = sdFont ? sdFont->getVertGlyph(cp, style) : nullptr;
if (vertGlyph) {
    // 縦書き用代替グリフで描画（メトリクス・ビットマップが縦書き用に設計済み）
    renderGlyph(vertGlyph, vertBitmap, x, yPos, black);
} else {
    // 通常グリフを正立で描画（CJK漢字等はvertなしで正しい）
    drawText(effectiveFontId, x, yPos, charBuf, black, style);
}
```

### 5. ビルトインフォント（Flash）への対応

- `fontconvert.py`の`--2bit`出力（`.h`ファイル生成）にもvertデータを含める
- `EpdFontData`構造体に追加:
  ```cpp
  const uint32_t* vertCodepoints;   // vert対象codepointの配列（ソート済み）
  const EpdGlyph* vertGlyphs;       // vert用グリフメトリクス
  const uint8_t* vertBitmaps;       // vert用ビットマップ（連結）
  uint16_t vertCount;               // vert対象グリフ数
  ```
- Flash配置（`static const`/`constexpr`）でRAM消費なし
- `GfxRenderer`にvertグリフ検索メソッドを追加（ビルトインフォント用）

### 6. VerticalTextUtils.h の変更

- 句読点テーブル（`VERTICAL_PUNCTUATION`）は不要になる
  - vertグリフの有無がそのまま「この文字は縦書きで別グリフを使う」のシグナル
  - テーブルは将来的に削除可能（vertフォント非対応のフォールバック用に残しても良い）

## メモリ影響

| 項目 | メモリ | 配置 |
|---|---|---|
| vertCodepoints + vertGlyphs（ビルトイン） | ~3.6KB/スタイル | Flash |
| vertBitmaps（ビルトイン） | ~5-10KB/スタイル | Flash |
| miniVertGlyphs（SDカード、prewarm時） | ~1.8KB/スタイル | Heap（一時的） |
| miniVertBitmap（SDカード、prewarm時） | ~3-5KB/スタイル | Heap（一時的） |

ビルトインフォントのvertデータはFlash配置のためRAM消費なし。
SDカードフォントのvertデータはprewarm時のみHeap使用（既存のminiGlyphsと同じライフサイクル）。

## テスト方針

- `fontconvert.py`で NotoSansCJK Regular の`vert`マッピングを確認
  - 期待: 「。」「、」「「」「」」「ー」等に代替グリフが存在
- 生成された`.cpfont`をバイナリ検査し vertセクションの構造を確認
- 実機テスト: 縦書きEPUBで句読点・括弧が正しい位置・形状で表示されることを確認
- 横書き回帰テスト: 横書きEPUBに影響がないことを確認
- v4 .cpfontファイルとの後方互換テスト: v4ファイルで`hasVert=false`として動作すること
