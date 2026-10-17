#pragma once

#include <string>

#include "activities/Activity.h"

// Shown while opening the digest fetches today's copy; Back skips to the saved
// one. Opens the reader afterwards, via a silent reboot if WiFi was used.
class DigestFetchActivity final : public Activity {
  std::string bookPath;
  bool allowFastInitialRefresh;
  bool fetched = false;

 public:
  explicit DigestFetchActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                               const bool allowFastInitialRefresh)
      : Activity("DigestFetch", renderer, mappedInput),
        bookPath(std::move(bookPath)),
        allowFastInitialRefresh(allowFastInitialRefresh) {}
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }
};
