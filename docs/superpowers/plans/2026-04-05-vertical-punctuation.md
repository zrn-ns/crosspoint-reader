# OpenType `vert` Vertical Punctuation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract OpenType `vert` feature glyph substitutions during font conversion, store them in `.cpfont` files, and use them at render time for proper vertical punctuation/bracket display.

**Architecture:** Three-layer change: (1) fontconvert.py extracts vert mappings from GSUB and writes a vert section per style in the .cpfont binary, (2) SdCardFont reads the vert section and provides getVertGlyph() lookup, (3) drawTextVertical() checks for vert substitutes before drawing each character.

**Tech Stack:** Python 3 (fontTools, FreeType), C++20, ESP32-C3 PlatformIO, custom .cpfont binary format.

**Testing:** Regenerate NotoSansCJK .cpfont with vert data, flash to device, verify 。、「」ー display correctly in vertical mode.

---

## File Map

| Action | File | Responsibility |
|--------|------|----------------|
| Modify | `lib/EpdFont/scripts/fontconvert.py` | Extract vert mappings, write vert section to .cpfont |
| Modify | `lib/EpdFont/SdCardFont.h` | Add vert fields to PerStyle, declare getVertGlyph() |
| Modify | `lib/EpdFont/SdCardFont.cpp` | Read vert section in load(), implement getVertGlyph(), extend prewarm |
| Modify | `lib/EpdFont/EpdFontData.h` | Add vert fields to EpdFontData (for builtin fonts) |
| Modify | `lib/GfxRenderer/GfxRenderer.cpp` | Use getVertGlyph() in drawTextVertical() |

---

## Task 1: fontconvert.py — Extract vert mappings and write vert section

**Files:**
- Modify: `lib/EpdFont/scripts/fontconvert.py`

- [ ] **Step 1: Add vert extraction function**

After the existing `load_glyph()` function (around line 180), add:

```python
def extract_vert_mappings(font_paths):
    """Extract codepoint -> substitute glyph name mapping from OpenType 'vert' feature.
    Returns dict {codepoint: substitute_glyph_name}."""
    for path in font_paths:
        try:
            tt = TTFont(path)
        except Exception:
            continue
        if 'GSUB' not in tt:
            continue
        gsub = tt['GSUB'].table
        cmap = tt.getBestCmap()
        if not cmap:
            continue
        reverse_cmap = {v: k for k, v in cmap.items()}

        vert_map = {}
        for feature in gsub.FeatureList.FeatureRecord:
            if feature.FeatureTag == 'vert':
                for lookup_idx in feature.Feature.LookupListIndex:
                    lookup = gsub.LookupList.Lookup[lookup_idx]
                    for subtable in lookup.SubTable:
                        if hasattr(subtable, 'mapping'):
                            vert_map.update(subtable.mapping)

        # Convert glyph names to codepoints
        result = {}
        for orig_name, sub_name in vert_map.items():
            if orig_name in reverse_cmap:
                cp = reverse_cmap[orig_name]
                result[cp] = sub_name
        if result:
            tt.close()
            return result
        tt.close()
    return {}
```

- [ ] **Step 2: Generate vert glyph data after main glyph generation**

After the main glyph loop (around line 310, after `all_glyphs` is built), add vert glyph generation:

```python
# --- Vertical glyph substitution (vert feature) ---
vert_mappings = extract_vert_mappings(args.fontstack)
vert_glyphs = []  # list of (codepoint, GlyphProps, packed_bitmap)

if vert_mappings:
    print(f"  vert: {len(vert_mappings)} substitutions found", file=sys.stderr)
    for cp, sub_glyph_name in sorted(vert_mappings.items()):
        # Load the substitute glyph by name (not by codepoint)
        rendered = False
        for face_idx, face in enumerate(font_stack):
            glyph_index = face.get_name_index(sub_glyph_name.encode('ascii', errors='replace'))
            if glyph_index == 0:
                # Try fontTools glyph order to get index
                try:
                    tt = TTFont(args.fontstack[face_idx])
                    glyph_order = tt.getGlyphOrder()
                    if sub_glyph_name in glyph_order:
                        glyph_index = glyph_order.index(sub_glyph_name)
                    tt.close()
                except Exception:
                    continue
            if glyph_index > 0:
                face.load_glyph(glyph_index, load_flags)
                bitmap = face.glyph.bitmap
                # Generate bitmap using same logic as main glyphs (2bit or 1bit)
                # ... (reuse the bitmap generation code from the main loop)
                # For brevity, extract bitmap generation into a helper function
                packed = render_glyph_bitmap(bitmap, is2Bit)
                glyph = GlyphProps(
                    width=bitmap.width,
                    height=bitmap.rows,
                    advance_x=fp4_from_ft16_16(face.glyph.linearHoriAdvance),
                    left=face.glyph.bitmap_left,
                    top=face.glyph.bitmap_top,
                    data_length=len(packed),
                    data_offset=0,  # will be set during output
                    code_point=cp,
                )
                vert_glyphs.append((cp, glyph, packed))
                rendered = True
                break
        if not rendered:
            print(f"  vert: WARNING: could not render substitute for U+{cp:04X} ({sub_glyph_name})", file=sys.stderr)

    print(f"  vert: {len(vert_glyphs)} glyphs rendered", file=sys.stderr)
```

Note: Extract the bitmap rendering code (lines 213-293) into a `render_glyph_bitmap(bitmap, is2Bit)` helper function to avoid duplication.

- [ ] **Step 3: Write vert section in .cpfont binary output**

In the `.cpfont` binary output section of fontconvert.py, after writing the main glyph data for each style:

- Set flag bit1 in the flags field: `flags |= 0x02` if `len(vert_glyphs) > 0`
- After the main style data, write the vert section:
  ```
  uint16_t vertCount
  For each vert glyph (sorted by codepoint):
    uint32_t codepoint
    EpdGlyph (14 bytes: width, height, advanceX, left, top, dataLength, dataOffset)
  uint8_t[] vertBitmaps (concatenated)
  ```
- Record `vertSectionOffset` in the style TOC

- [ ] **Step 4: Update .cpfont version**

Change `CPFONT_VERSION` from 4 to 5 in the output code.

- [ ] **Step 5: Write vert data in .h header output (builtin fonts)**

For the C header output path, add vert arrays:
```c
static const uint32_t {name}VertCodepoints[] = { 0x3001, 0x3002, ... };
static const EpdGlyph {name}VertGlyphs[] = { {w, h, ax, l, t, dl, do}, ... };
static const uint8_t {name}VertBitmaps[] = { 0x00, ... };
```
And add vert fields to the EpdFontData struct initialization.

- [ ] **Step 6: Test locally**

```bash
cd lib/EpdFont/scripts
python3 fontconvert.py test_vert 16 /path/to/NotoSansCJKjp-Regular.otf --2bit 2>&1 | grep vert
```
Expected: `vert: N substitutions found` and `vert: M glyphs rendered`

- [ ] **Step 7: Commit**

```bash
git add lib/EpdFont/scripts/fontconvert.py
git commit -m "✨ fontconvert.pyにOpenType vert代替グリフの抽出・出力を追加"
```

---

## Task 2: EpdFontData — Add vert fields for builtin fonts

**Files:**
- Modify: `lib/EpdFont/EpdFontData.h`

- [ ] **Step 1: Add vert fields to EpdFontData struct**

After `ligaturePairCount` (line 98), add:

```cpp
  // Vertical glyph substitution (from OpenType 'vert' GSUB feature).
  // Used in vertical text mode for CJK punctuation/brackets.
  const uint32_t* vertCodepoints;     ///< Sorted codepoint array for binary search (nullptr if none)
  const EpdGlyph* vertGlyphs;         ///< Corresponding vertical substitute glyphs
  const uint8_t* vertBitmaps;         ///< Concatenated bitmap data for vert glyphs
  uint16_t vertCount;                 ///< Number of vert substitution entries (0 if none)
```

- [ ] **Step 2: Build verification**

```bash
pio run 2>&1 | tail -5
```

Expected: Build may produce warnings about missing initializers in existing EpdFontData initializations. Fix them by adding `, nullptr, nullptr, nullptr, 0` to existing static EpdFontData declarations.

- [ ] **Step 3: Fix any initializer warnings in builtin font headers**

Search for all `static const EpdFontData` initializations and add the 4 new zero/nullptr fields.

- [ ] **Step 4: Commit**

```bash
git add lib/EpdFont/EpdFontData.h lib/EpdFont/builtinFonts/
git commit -m "✨ EpdFontDataに縦書き代替グリフ（vert）フィールドを追加"
```

---

## Task 3: SdCardFont — Read vert section and provide lookup

**Files:**
- Modify: `lib/EpdFont/SdCardFont.h`
- Modify: `lib/EpdFont/SdCardFont.cpp`

- [ ] **Step 1: Add vert fields to PerStyle struct in SdCardFont.h**

Inside the `PerStyle` struct (after `miniGlyphCount`, around line 127):

```cpp
    // Vertical glyph substitution data (from .cpfont vert section)
    uint32_t vertSectionOffset = 0;   // File offset to vert section (0 = no vert data)
    uint16_t vertCount = 0;           // Number of vert entries

    // Prewarm'd vert glyph data (loaded during prewarm when vertical mode)
    uint32_t* vertCodepoints = nullptr;
    EpdGlyph* vertGlyphs = nullptr;
    uint8_t* vertBitmap = nullptr;
    bool vertLoaded = false;
```

- [ ] **Step 2: Add public methods to SdCardFont.h**

```cpp
  // Look up vertical substitute glyph for a codepoint.
  // Returns the vert EpdGlyph* if found, nullptr otherwise.
  const EpdGlyph* getVertGlyph(uint32_t codepoint, uint8_t style = 0) const;

  // Returns the bitmap data for a vert glyph (must be from getVertGlyph).
  const uint8_t* getVertBitmap(const EpdGlyph* vertGlyph, uint8_t style = 0) const;

  // Returns true if this font has vert data for any style.
  bool hasVertData() const;
```

- [ ] **Step 3: Read vert section offset in SdCardFont::load()**

In `SdCardFont.cpp`, in the `load()` method where the style TOC is read, parse the `vertSectionOffset` from the TOC entry. The .cpfont v5 format stores it in the style TOC.

Check the flags field: `bool hasVert = (flags & 0x02) != 0;`

If `hasVert`, read `vertSectionOffset` from the style TOC. Also read `vertCount` from the vert section header (seek to offset, read uint16_t).

- [ ] **Step 4: Implement getVertGlyph() with binary search**

```cpp
const EpdGlyph* SdCardFont::getVertGlyph(uint32_t codepoint, uint8_t style) const {
  if (style >= MAX_STYLES || !styles_[style].vertLoaded || styles_[style].vertCount == 0)
    return nullptr;

  const auto& s = styles_[style];
  // Binary search on sorted codepoint array
  uint32_t lo = 0, hi = s.vertCount;
  while (lo < hi) {
    uint32_t mid = lo + (hi - lo) / 2;
    if (s.vertCodepoints[mid] < codepoint) lo = mid + 1;
    else hi = mid;
  }
  if (lo < s.vertCount && s.vertCodepoints[lo] == codepoint) {
    return &s.vertGlyphs[lo];
  }
  return nullptr;
}
```

- [ ] **Step 5: Implement getVertBitmap()**

```cpp
const uint8_t* SdCardFont::getVertBitmap(const EpdGlyph* vertGlyph, uint8_t style) const {
  if (!vertGlyph || style >= MAX_STYLES || !styles_[style].vertBitmap) return nullptr;
  return styles_[style].vertBitmap + vertGlyph->dataOffset;
}
```

- [ ] **Step 6: Load vert data during prewarm when vertical mode**

In `prewarmStyle()`, after loading normal mini glyphs/bitmaps, if `vertSectionOffset != 0` and the caller signals vertical mode, load vert data:

1. Seek to `vertSectionOffset`
2. Read `vertCount`, codepoints array, EpdGlyph array, bitmap data
3. Store in `vertCodepoints`, `vertGlyphs`, `vertBitmap`
4. Set `vertLoaded = true`

Add a `bool verticalMode` parameter to `prewarm()` (default false). When true and vert data exists, also load vert section.

- [ ] **Step 7: Free vert data in freeStyleMiniData()**

```cpp
void SdCardFont::freeStyleMiniData(PerStyle& s) {
  // ... existing cleanup ...
  delete[] s.vertCodepoints; s.vertCodepoints = nullptr;
  delete[] s.vertGlyphs; s.vertGlyphs = nullptr;
  delete[] s.vertBitmap; s.vertBitmap = nullptr;
  s.vertLoaded = false;
}
```

- [ ] **Step 8: Implement hasVertData()**

```cpp
bool SdCardFont::hasVertData() const {
  for (uint8_t i = 0; i < MAX_STYLES; i++) {
    if (styles_[i].present && styles_[i].vertSectionOffset != 0) return true;
  }
  return false;
}
```

- [ ] **Step 9: Build verification**

```bash
pio run 2>&1 | tail -5
```

Expected: SUCCESS

- [ ] **Step 10: Commit**

```bash
git add lib/EpdFont/SdCardFont.h lib/EpdFont/SdCardFont.cpp
git commit -m "✨ SdCardFontにvert代替グリフの読み込み・検索機能を追加"
```

---

## Task 4: drawTextVertical — Use vert glyphs

**Files:**
- Modify: `lib/GfxRenderer/GfxRenderer.cpp`

- [ ] **Step 1: Modify drawTextVertical() to check for vert substitutes**

In `drawTextVertical()`, before the current upright draw call, check for a vert glyph:

```cpp
    // Check for vertical substitute glyph (OpenType 'vert' feature)
    auto sdIt = sdCardFonts_.find(effectiveFontId);
    const EpdGlyph* vertGlyph = nullptr;
    const uint8_t* vertBitmap = nullptr;
    if (sdIt != sdCardFonts_.end()) {
      vertGlyph = sdIt->second->getVertGlyph(cp, static_cast<uint8_t>(style));
      if (vertGlyph) {
        vertBitmap = sdIt->second->getVertBitmap(vertGlyph, static_cast<uint8_t>(style));
      }
    }

    if (vertGlyph && vertBitmap) {
      // Draw using vertical substitute glyph (proper CJK vertical punctuation)
      const int vAdvance = fp4::toPixel(vertGlyph->advanceX);
      // Render the vert glyph bitmap directly (same pixel loop as renderChar)
      const int glyphX = x + vertGlyph->left;
      const int glyphY = yPos + getFontAscenderSize(effectiveFontId) - vertGlyph->top;
      renderBitmap(vertGlyph, vertBitmap, fontData->is2Bit, glyphX, glyphY, black);
      yPos += vAdvance + vAdvance / 10;
    } else {
      // Normal upright character (existing code)
      drawText(effectiveFontId, x, yPos, charBuf, black, style);
      yPos += verticalAdvance;
    }
```

Note: `renderBitmap()` is a helper that draws an EpdGlyph's bitmap at screen coordinates. Extract the pixel loop from the existing `renderChar` template (GfxRenderer.cpp line 230-312) into a callable helper, or call the template directly.

- [ ] **Step 2: Pass verticalMode to prewarm**

In `FontCacheManager::prewarmCache()` and the prewarm scope, pass `verticalMode` through to `SdCardFont::prewarm()` so vert data gets loaded during the scan pass.

This requires adding a `bool verticalMode` parameter to:
- `FontCacheManager::prewarmCache()`
- `FontCacheManager::PrewarmScope` (store it, pass during endScanAndPrewarm)
- `SdCardFont::prewarm()`

- [ ] **Step 3: Wire verticalMode through EpubReaderActivity**

In `EpubReaderActivity::renderContents()`, pass `verticalMode` to the prewarm scope creation. The PrewarmScope stores it and passes to prewarmCache when endScanAndPrewarm() is called.

- [ ] **Step 4: Build verification**

```bash
pio run 2>&1 | tail -5
```

Expected: SUCCESS

- [ ] **Step 5: Commit**

```bash
git add lib/GfxRenderer/GfxRenderer.cpp lib/GfxRenderer/GfxRenderer.h \
  lib/GfxRenderer/FontCacheManager.cpp lib/GfxRenderer/FontCacheManager.h \
  src/activities/reader/EpubReaderActivity.cpp
git commit -m "✨ drawTextVerticalでvert代替グリフを優先使用"
```

---

## Task 5: Regenerate fonts and end-to-end test

**Files:** No new code — font regeneration and device testing.

- [ ] **Step 1: Regenerate NotoSansCJK .cpfont with vert data**

Run `fontconvert.py` with the NotoSansCJK font to generate a new .cpfont that includes vert data. Upload to device SD card.

Verify the output logs show vert glyph extraction:
```
vert: N substitutions found
vert: M glyphs rendered
```

- [ ] **Step 2: Flash firmware and test**

```bash
pio run -t upload --upload-port /dev/tty.usbmodem101
```

- [ ] **Step 3: Device verification**

1. Clear reading cache
2. Set writing mode to "縦書き"
3. Open a Japanese EPUB with 。、「」ー characters
4. Verify punctuation/brackets display with correct vertical forms
5. Switch to "横書き" — verify no regression
6. Test with "自動" — verify auto-detection still works

- [ ] **Step 4: Commit any fixes**

```bash
git add -A
git commit -m "🐛 vert縦書きグリフの統合テストで発見した問題を修正"
```

---

## Done Criteria

- [ ] `fontconvert.py` がOpenType `vert`マッピングを抽出し`.cpfont`に格納する
- [ ] `.cpfont` v5フォーマットにvertセクションが含まれる
- [ ] `SdCardFont::getVertGlyph()` が二分探索でvert代替グリフを返す
- [ ] `drawTextVertical()` がvert代替グリフを優先使用する
- [ ] 句読点（。、）、括弧（「」）、長音（ー）が縦書き用グリフで表示される
- [ ] 横書き表示に回帰バグがない
- [ ] v4 .cpfontファイルで`hasVert=false`として動作する（後方互換）

※ 必須ゲート（動作検証・既存機能・差分確認・シークレット）は常に適用
