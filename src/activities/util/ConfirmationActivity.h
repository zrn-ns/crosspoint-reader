#pragma once
#include <functional>
#include <string>

#include "../../fontIds.h"
#include "../Activity.h"

class ConfirmationActivity : public Activity {
 private:
  // Input data
  std::string heading;
  std::string body;

  const int margin = 20;
  const int spacing = 30;
  const int fontId = UI_10_FONT_ID;

  std::string safeHeading;
  std::string safeBody;
  std::string neverLabel;
  std::string confirmLabel;
  std::string backLabel;
  int startY = 0;
  int lineHeight = 0;

 public:
  // resultCode in ActivityResult::data: 0=confirm, 1=cancel, 2=never
  static constexpr int RESULT_NEVER = 2;

  ConfirmationActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const std::string& heading,
                       const std::string& body, const std::string& neverLabel = "",
                       const std::string& confirmLabel = "", const std::string& backLabel = "");

  void onEnter() override;
  void loop() override;
  void render(RenderLock&& lock) override;
};