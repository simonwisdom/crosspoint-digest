#include "DigestFetch.h"

#include <HalStorage.h>
#include <LibraryBuilder.h>
#include <Logging.h>
#include <WiFi.h>

#include "WifiCredentialStore.h"
#include "network/HttpDownloader.h"
#include "util/BookCacheUtils.h"

namespace DigestFetch {
namespace {

constexpr const char* DIGEST_DIR = "/Digest";
constexpr const char* PART_PATH = "/Digest/.Daily Digest.part";
// The digest address lives on the SD card, not in the firmware: release builds
// are public, and the address's random path is all that keeps the digest private.
constexpr const char* URL_PATH = "/.crosspoint/digest-url";
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000;

// RTC_NOINIT survives the silent reboot that follows a fetch (RTC_DATA_ATTR is
// re-initialised by ESP.restart()); startNewWake() clears it on every real wake.
constexpr uint32_t TRIED_MAGIC = 0xD16E57A1;
RTC_NOINIT_ATTR uint32_t triedThisWakeMagic;

bool cancelled(const CancelCheck& cancel) { return cancel.fn && cancel.fn(cancel.ctx); }

bool inRange(const std::string& ssid, const int found) {
  for (int i = 0; i < found; i++) {
    if (WiFi.SSID(i) == ssid.c_str()) return true;
  }
  return false;
}

// Joins the last connected saved network first, then the rest in saved order,
// skipping any the scan didn't see so an away-from-home wake gives up quickly.
bool connectToSavedWifi(const CancelCheck& cancel) {
  WIFI_STORE.loadFromFile();
  const size_t count = WIFI_STORE.getCredentialCount();
  if (count == 0) {
    LOG_INF("DIG", "No saved WiFi networks");
    return false;
  }

  WiFi.mode(WIFI_STA);
  const int found = WiFi.scanNetworks();
  const std::string last = WIFI_STORE.getLastConnectedSsid();
  bool connected = false;
  for (size_t pass = 0; pass < 2 && !connected; pass++) {
    for (size_t i = 0; i < count && !connected; i++) {
      const auto cred = WIFI_STORE.getCredentialAt(i);
      if (!cred) continue;
      const bool isLast = !last.empty() && cred->ssid == last;
      if ((pass == 0) != isLast || !inRange(cred->ssid, found)) continue;
      LOG_INF("DIG", "Connecting to %s", cred->ssid.c_str());
      if (cred->password.empty()) {
        WiFi.begin(cred->ssid.c_str());
      } else {
        WiFi.begin(cred->ssid.c_str(), cred->password.c_str());
      }
      const unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
        if (cancelled(cancel)) break;
        delay(100);
      }
      connected = WiFi.status() == WL_CONNECTED;
      if (!connected) WiFi.disconnect();
      if (cancelled(cancel)) break;
    }
  }
  WiFi.scanDelete();
  if (!connected) LOG_INF("DIG", "No saved WiFi network joined (%d in range)", found);
  return connected;
}

bool downloadDigest(const String& url, const CancelCheck& cancel) {
  if (!Storage.ensureDirectoryExists(DIGEST_DIR)) {
    LOG_ERR("DIG", "Cannot create %s", DIGEST_DIR);
    return false;
  }
  LOG_INF("DIG", "Downloading digest (free heap %u)", ESP.getFreeHeap());
  bool cancelFlag = false;
  // Download beside the digest first: downloadToFile() deletes its target up
  // front, and a failed fetch must leave the previous digest readable.
  const auto result = HttpDownloader::downloadToFile(
      url.c_str(), PART_PATH, [&](size_t, size_t) { cancelFlag = cancelled(cancel); }, &cancelFlag);
  if (result != HttpDownloader::OK) {
    LOG_ERR("DIG", "Download failed (%d)", static_cast<int>(result));
    Storage.remove(PART_PATH);
    return false;
  }
  if (Storage.exists(DIGEST_PATH) && !Storage.remove(DIGEST_PATH)) {
    LOG_ERR("DIG", "Cannot remove old digest");
    Storage.remove(PART_PATH);
    return false;
  }
  if (!Storage.rename(PART_PATH, DIGEST_PATH)) {
    LOG_ERR("DIG", "Cannot rename downloaded digest");
    return false;
  }
  clearBookCache(DIGEST_PATH);
  library::markLibraryIndexDirty();
  LOG_INF("DIG", "Digest updated");
  return true;
}

}  // namespace

void startNewWake() { triedThisWakeMagic = 0; }

bool shouldFetchBeforeOpening(const std::string& path) {
#ifdef DIGEST_FETCH
  return triedThisWakeMagic != TRIED_MAGIC && path == DIGEST_PATH;
#else
  (void)path;
  return false;
#endif
}

bool fetch(const CancelCheck cancel) {
  triedThisWakeMagic = TRIED_MAGIC;
  String url = Storage.readFile(URL_PATH);
  url.trim();
  if (!url.startsWith("https://")) {
    LOG_ERR("DIG", "No https address in %s", URL_PATH);
    return false;
  }

  if (connectToSavedWifi(cancel) && !cancelled(cancel)) {
    downloadDigest(url, cancel);
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return true;
}

}  // namespace DigestFetch
