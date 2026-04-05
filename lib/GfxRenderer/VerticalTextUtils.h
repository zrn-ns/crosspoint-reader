#pragma once

#include <cstdint>

namespace VerticalTextUtils {

// Character behavior in vertical text layout
enum class VerticalBehavior : uint8_t {
  Upright,      // CJK ideographs, kana - draw normally, advance downward
  Sideways,     // Latin letters, 3+ digit numbers - rotate 90 CW
  TateChuYoko,  // 1-2 digit numbers - horizontal-in-vertical
};

// Punctuation offset for vertical text (ratio of character size, in 1/8 units)
struct PunctuationOffset {
  uint32_t codepoint;
  int8_t dxEighths;  // horizontal offset in 1/8 of charWidth
  int8_t dyEighths;  // vertical offset in 1/8 of charHeight
  bool rotate;       // true = rotate 90 CW (e.g. long vowel mark)
};

// Punctuation that needs repositioning in vertical text.
// Offsets are in 1/8 of character dimension to avoid floating point.
static constexpr PunctuationOffset VERTICAL_PUNCTUATION[] = {
    {0x3001, 3, -3, false},   // 、 ideographic comma -> upper-right
    {0x3002, 3, -3, false},   // 。 ideographic period -> upper-right
    {0xFF0C, 3, -3, false},   // ， fullwidth comma -> upper-right
    {0xFF0E, 3, -3, false},   // ． fullwidth period -> upper-right
    {0x30FC, 0, 0, true},     // ー katakana long vowel mark -> rotate
    {0x2014, 0, 0, true},     // — em dash -> rotate
    {0x2015, 0, 0, true},     // ― horizontal bar -> rotate
    {0x2026, 0, 0, true},     // … ellipsis -> rotate
    {0xFF5E, 0, 0, true},     // ～ fullwidth tilde -> rotate
};
static constexpr int VERTICAL_PUNCTUATION_COUNT =
    sizeof(VERTICAL_PUNCTUATION) / sizeof(VERTICAL_PUNCTUATION[0]);

// Look up punctuation offset. Returns nullptr if no special handling needed.
inline const PunctuationOffset* getVerticalPunctuationOffset(uint32_t cp) {
  for (int i = 0; i < VERTICAL_PUNCTUATION_COUNT; i++) {
    if (VERTICAL_PUNCTUATION[i].codepoint == cp) return &VERTICAL_PUNCTUATION[i];
  }
  return nullptr;
}

// Determine if a codepoint should be drawn upright in vertical text.
// CJK ideographs, kana, CJK symbols, fullwidth forms, etc.
inline bool isUprightInVertical(uint32_t cp) {
  if (cp >= 0x4E00 && cp <= 0x9FFF) return true;   // CJK Unified Ideographs
  if (cp >= 0x3400 && cp <= 0x4DBF) return true;   // CJK Extension A
  if (cp >= 0x3040 && cp <= 0x309F) return true;   // Hiragana
  if (cp >= 0x30A0 && cp <= 0x30FF) return true;   // Katakana
  if (cp >= 0x3000 && cp <= 0x303F) return true;   // CJK Symbols and Punctuation
  if (cp >= 0xFF00 && cp <= 0xFFEF) return true;   // Fullwidth Forms
  if (cp >= 0xF900 && cp <= 0xFAFF) return true;   // CJK Compatibility Ideographs
  if (cp >= 0x3200 && cp <= 0x32FF) return true;   // Enclosed CJK Letters
  if (cp >= 0x3300 && cp <= 0x33FF) return true;   // CJK Compatibility
  if (cp >= 0x3100 && cp <= 0x312F) return true;   // Bopomofo
  if (cp >= 0xAC00 && cp <= 0xD7AF) return true;   // Hangul
  return false;
}

}  // namespace VerticalTextUtils
