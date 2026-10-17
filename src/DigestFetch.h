#pragma once

#include <string>

// Daily digest: opening the digest first joins a saved WiFi network in range
// and downloads the address in /.crosspoint/digest-url over the previous
// digest, once per wake. Sleep stays stock (battery power-off), so the device
// has no clock and checks the server on each wake instead. Enabled by defining
// DIGEST_FETCH at build time; without it the digest opens like any book.
namespace DigestFetch {

constexpr const char* DIGEST_PATH = "/Digest/Daily Digest.epub";

// Polled while waiting on the network; returning true abandons the fetch.
struct CancelCheck {
  void* ctx;
  bool (*fn)(void* ctx);
};

// Call once per wake (not on a silent reboot, which continues the same wake).
void startNewWake();

// True when opening path should fetch first: it is the digest and this wake
// hasn't tried yet.
bool shouldFetchBeforeOpening(const std::string& path);

// Connect, download, WiFi off. Counts as this wake's attempt whatever the
// outcome; a failed or cancelled download keeps the previous digest in place.
// Returns true if the WiFi radio was used, so the caller can clear the heap.
bool fetch(CancelCheck cancel);

}  // namespace DigestFetch
