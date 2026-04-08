#include "GrayscaleTestActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <Logging.h>

GrayscaleTestActivity::GrayscaleTestActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("GrayTest", renderer, mappedInput) {}

void GrayscaleTestActivity::onEnter() {
  Activity::onEnter();
  renderTestPattern();
}

void GrayscaleTestActivity::onExit() { Activity::onExit(); }

void GrayscaleTestActivity::loop() {
  Activity::loop();

  if (mappedInput.wasReleased(MappedInputManager::Button::Back) ||
      mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    onGoHome();
  }
}

void GrayscaleTestActivity::renderTestPattern() {
  const int w = renderer.getScreenWidth();
  const int h = renderer.getScreenHeight();
  const int stripeW = w / 4;

  LOG_INF("GRAY", "Test pattern: %dx%d, stripe=%d", w, h, stripeW);

  // --- Pass 1: BW --- 全ての黒+灰色ピクセルを黒として描画
  renderer.clearScreen();  // 0xFF = 全白
  // val=0 (黒): ストライプ0 — drawPixelで黒化
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < stripeW; x++) {
      renderer.drawPixel(x, y);  // state=true → 黒
    }
  }
  // val=1 (濃灰): ストライプ1 — BWパスでも黒として描画
  for (int y = 0; y < h; y++) {
    for (int x = stripeW; x < stripeW * 2; x++) {
      renderer.drawPixel(x, y);
    }
  }
  // val=2 (薄灰): ストライプ2 — BWパスでも黒として描画
  for (int y = 0; y < h; y++) {
    for (int x = stripeW * 2; x < stripeW * 3; x++) {
      renderer.drawPixel(x, y);
    }
  }
  // val=3 (白): ストライプ3 — そのまま白

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
  LOG_INF("GRAY", "BW pass done");

  // --- Pass 2: LSB --- 濃灰(val=1)のみマーク
  renderer.clearScreen(0x00);  // 全ビット0
  renderer.setRenderMode(GfxRenderer::GRAYSCALE_LSB);
  for (int y = 0; y < h; y++) {
    for (int x = stripeW; x < stripeW * 2; x++) {
      renderer.drawPixel(x, y, false);  // ビットを1にセット
    }
  }
  renderer.copyGrayscaleLsbBuffers();
  LOG_INF("GRAY", "LSB pass done");

  // --- Pass 3: MSB --- 濃灰(val=1) + 薄灰(val=2)をマーク
  renderer.clearScreen(0x00);
  renderer.setRenderMode(GfxRenderer::GRAYSCALE_MSB);
  for (int y = 0; y < h; y++) {
    for (int x = stripeW; x < stripeW * 3; x++) {
      renderer.drawPixel(x, y, false);
    }
  }
  renderer.copyGrayscaleMsbBuffers();
  LOG_INF("GRAY", "MSB pass done");

  // --- Display grayscale ---
  renderer.displayGrayBuffer();
  renderer.setRenderMode(GfxRenderer::BW);
  LOG_INF("GRAY", "Grayscale display done");
}
