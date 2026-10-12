#pragma once

#include <cstdint>

namespace DigestSchedule {

constexpr int64_t DAY_S = 86400;
// Wake soon when sleep starts after a fetch time the device slept through or
// was awake for, so the day's digest isn't skipped.
constexpr int64_t MISSED_FETCH_DELAY_S = 60;
// After a failed fetch, try again a few times before waiting for tomorrow.
constexpr int64_t RETRY_DELAY_S = 30 * 60;
constexpr int MAX_FAILED_ATTEMPTS = 4;

// Seconds from now until the next fetch.
//   secondsIntoDay: local time of day now, 0..86399
//   fetchSecondOfDay: local fetch time, 0..86399
//   fetchedSinceTodaysTime: a fetch succeeded at or after today's fetch time
//   failedAttemptsToday: failed fetches since today's fetch time
constexpr int64_t secondsUntilFetch(const int64_t secondsIntoDay, const int64_t fetchSecondOfDay,
                                    const bool fetchedSinceTodaysTime, const int failedAttemptsToday) {
  if (secondsIntoDay >= fetchSecondOfDay && !fetchedSinceTodaysTime) {
    if (failedAttemptsToday == 0) return MISSED_FETCH_DELAY_S;
    if (failedAttemptsToday < MAX_FAILED_ATTEMPTS) return RETRY_DELAY_S;
  }
  const int64_t wait = (fetchSecondOfDay - secondsIntoDay + DAY_S) % DAY_S;
  return wait == 0 ? DAY_S : wait;
}

}  // namespace DigestSchedule
