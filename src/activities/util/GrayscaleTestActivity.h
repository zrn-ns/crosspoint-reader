#pragma once

#include "../Activity.h"
#include "MappedInputManager.h"

// 4階調グレースケールテスト用Activity
// GRAYSCALE_TEST_MODE マクロが定義されている場合のみコンパイルされる。
// 画面を4つの縦ストライプ（黒/濃灰/薄灰/白）で描画し、
// X3のグレースケールLUT動作を目視確認するためのデバッグツール。
class GrayscaleTestActivity final : public Activity {
 public:
  GrayscaleTestActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;

 private:
  void renderTestPattern();
};
