#include "DigestFetchActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "CrossPointState.h"
#include "DigestFetch.h"
#include "SilentRestart.h"
#include "components/UITheme.h"
#include "fontIds.h"

void DigestFetchActivity::loop() {
  if (fetched) return;
  fetched = true;
  requestUpdateAndWait();

  const bool usedWifi = DigestFetch::fetch({&mappedInput, [](void* ctx) {
                                              auto& input = *static_cast<MappedInputManager*>(ctx);
                                              input.update();
                                              return input.wasPressed(MappedInputManager::Button::Back);
                                            }});
  if (usedWifi) {
    // Same heap reset as the other WiFi screens; the reboot resumes this path.
    APP_STATE.openEpubPath = bookPath;
    APP_STATE.saveToFile();
    silentRestartToReader();
  }
  activityManager.goToReader(bookPath, allowFastInitialRefresh);
}

void DigestFetchActivity::render(RenderLock&&) {
  const auto top = (renderer.getScreenHeight() - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
  renderer.clearScreen();
  renderer.drawCenteredText(UI_10_FONT_ID, top, tr(STR_DIGEST_FETCHING));
  const auto labels = mappedInput.mapLabels(tr(STR_CANCEL), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
