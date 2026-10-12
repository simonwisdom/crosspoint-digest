#pragma once

#include <cstdint>

// Daily digest: once a day the device wakes on a timer, joins a saved WiFi
// network, downloads the address in /.crosspoint/digest-url over the previous
// digest and sleeps again. Enabled by defining DIGEST_FETCH at build time;
// without it every function here is a no-op and sleep is stock behaviour.
// The fetch time is DIGEST_HOUR:DIGEST_MINUTE in the configured timezone.
namespace DigestFetch {

constexpr const char* DIGEST_PATH = "/Digest/Daily Digest.epub";

bool enabled();

// Seconds to arm as a timer wakeup when entering deep sleep, or 0 for none.
uint64_t secondsUntilNextFetch();

// Timer-wake work: connect, sync the clock, download the digest. Leaves WiFi
// off. A failed download keeps the previous digest in place.
void runTimerWake();

}  // namespace DigestFetch
