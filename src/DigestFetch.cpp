#include "DigestFetch.h"

#include <HalClock.h>
#include <HalStorage.h>
#include <LibraryBuilder.h>
#include <Logging.h>
#include <WiFi.h>

#include <ctime>

#include "DigestSchedule.h"
#include "WifiCredentialStore.h"
#include "network/HttpDownloader.h"
#include "util/BookCacheUtils.h"

#ifndef DIGEST_HOUR
#define DIGEST_HOUR 6
#endif
#ifndef DIGEST_MINUTE
#define DIGEST_MINUTE 13
#endif

namespace DigestFetch {
namespace {

constexpr const char* DIGEST_DIR = "/Digest";
constexpr const char* PART_PATH = "/Digest/.Daily Digest.part";
// The digest address lives on the SD card, not in the firmware: release builds
// are public, and the address's random path is all that keeps the digest private.
constexpr const char* URL_PATH = "/.crosspoint/digest-url";
constexpr int64_t FETCH_SECOND_OF_DAY = DIGEST_HOUR * 3600 + DIGEST_MINUTE * 60;
// With no valid clock (first boot after a battery power-off) wake this often
// to try WiFi, which also sets the clock.
constexpr uint64_t NO_CLOCK_RETRY_S = 6 * 3600;
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000;

// RTC slow memory: survives deep sleep, zeroed on power-on.
RTC_DATA_ATTR time_t lastFetchEpoch = 0;
// Failed attempts counted against the fetch time in failedForFetchEpoch, so
// the count starts over each day.
RTC_DATA_ATTR int failedAttempts = 0;
RTC_DATA_ATTR time_t failedForFetchEpoch = 0;

// Local fetch time today as a UTC epoch, plus the local time of day now.
bool todaysFetch(time_t& fetchEpoch, int64_t& secondsIntoDay) {
  time_t now;
  struct tm local;
  if (!halClock.getEpoch(now) || !halClock.localTime(local)) return false;
  secondsIntoDay = local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
  fetchEpoch = now - secondsIntoDay + FETCH_SECOND_OF_DAY;
  return true;
}

bool connectToSavedWifi() {
  WIFI_STORE.loadFromFile();
  const size_t count = WIFI_STORE.getCredentialCount();
  if (count == 0) {
    LOG_INF("DIG", "No saved WiFi networks");
    return false;
  }

  WiFi.mode(WIFI_STA);
  const std::string last = WIFI_STORE.getLastConnectedSsid();
  // Try the last connected network first, then the rest in saved order.
  for (size_t pass = 0; pass < 2; pass++) {
    for (size_t i = 0; i < count; i++) {
      const auto cred = WIFI_STORE.getCredentialAt(i);
      if (!cred) continue;
      const bool isLast = !last.empty() && cred->ssid == last;
      if ((pass == 0) != isLast) continue;
      LOG_INF("DIG", "Connecting to %s", cred->ssid.c_str());
      if (cred->password.empty()) {
        WiFi.begin(cred->ssid.c_str());
      } else {
        WiFi.begin(cred->ssid.c_str(), cred->password.c_str());
      }
      const unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
        delay(100);
      }
      if (WiFi.status() == WL_CONNECTED) return true;
      WiFi.disconnect();
    }
  }
  return false;
}

#ifdef DIGEST_FETCH
bool downloadDigest() {
  String url = Storage.readFile(URL_PATH);
  url.trim();
  if (!url.startsWith("https://")) {
    LOG_ERR("DIG", "No https address in %s", URL_PATH);
    return false;
  }
  if (!Storage.ensureDirectoryExists(DIGEST_DIR)) {
    LOG_ERR("DIG", "Cannot create %s", DIGEST_DIR);
    return false;
  }
  LOG_INF("DIG", "Downloading digest (free heap %u)", ESP.getFreeHeap());
  // Download beside the digest first: downloadToFile() deletes its target up
  // front, and a failed fetch must leave yesterday's digest readable.
  const auto result = HttpDownloader::downloadToFile(url.c_str(), PART_PATH);
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
#endif

}  // namespace

#ifdef DIGEST_FETCH
bool enabled() { return true; }
#else
bool enabled() { return false; }
#endif

uint64_t secondsUntilNextFetch() {
  if (!enabled()) return 0;
  time_t fetchEpoch;
  int64_t secondsIntoDay;
  if (!todaysFetch(fetchEpoch, secondsIntoDay)) return NO_CLOCK_RETRY_S;

  const int failedToday = failedForFetchEpoch == fetchEpoch ? failedAttempts : 0;
  return static_cast<uint64_t>(DigestSchedule::secondsUntilFetch(secondsIntoDay, FETCH_SECOND_OF_DAY,
                                                                 lastFetchEpoch >= fetchEpoch, failedToday));
}

void runTimerWake() {
#ifdef DIGEST_FETCH
  bool ok = false;
  if (connectToSavedWifi()) {
    halClock.syncFromNTP();
    ok = downloadDigest();
  } else {
    LOG_ERR("DIG", "No WiFi, digest not updated");
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  time_t fetchEpoch;
  int64_t secondsIntoDay;
  if (!todaysFetch(fetchEpoch, secondsIntoDay)) return;
  if (ok) {
    halClock.getEpoch(lastFetchEpoch);
    failedAttempts = 0;
  } else if (secondsIntoDay >= FETCH_SECOND_OF_DAY) {
    failedAttempts = failedForFetchEpoch == fetchEpoch ? failedAttempts + 1 : 1;
    failedForFetchEpoch = fetchEpoch;
  }
#endif
}

}  // namespace DigestFetch
