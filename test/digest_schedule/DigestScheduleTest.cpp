#include <gtest/gtest.h>

#include "DigestSchedule.h"

using DigestSchedule::DAY_S;
using DigestSchedule::MISSED_FETCH_DELAY_S;
using DigestSchedule::RETRY_DELAY_S;
using DigestSchedule::secondsUntilFetch;

namespace {
constexpr int64_t AT_0613 = 6 * 3600 + 13 * 60;
}

TEST(DigestScheduleTest, WaitsUntilFetchTimeLaterToday) {
  EXPECT_EQ(secondsUntilFetch(5 * 3600, AT_0613, false, 0), 73 * 60);
}

TEST(DigestScheduleTest, AfterTodaysFetchWaitsUntilTomorrow) {
  EXPECT_EQ(secondsUntilFetch(22 * 3600, AT_0613, true, 0), DAY_S - 22 * 3600 + AT_0613);
}

TEST(DigestScheduleTest, AtFetchTimeAfterFetchingWaitsAFullDay) {
  EXPECT_EQ(secondsUntilFetch(AT_0613, AT_0613, true, 0), DAY_S);
}

TEST(DigestScheduleTest, MissedFetchRunsSoon) {
  EXPECT_EQ(secondsUntilFetch(9 * 3600, AT_0613, false, 0), MISSED_FETCH_DELAY_S);
}

TEST(DigestScheduleTest, FailedFetchRetriesLater) {
  EXPECT_EQ(secondsUntilFetch(AT_0613 + 20, AT_0613, false, 1), RETRY_DELAY_S);
  EXPECT_EQ(secondsUntilFetch(AT_0613 + 20, AT_0613, false, 3), RETRY_DELAY_S);
}

TEST(DigestScheduleTest, GivesUpUntilTomorrowAfterMaxFailures) {
  const int64_t now = 8 * 3600;
  EXPECT_EQ(secondsUntilFetch(now, AT_0613, false, DigestSchedule::MAX_FAILED_ATTEMPTS), DAY_S - now + AT_0613);
}

TEST(DigestScheduleTest, FailuresBeforeFetchTimeDoNotChangeTheWait) {
  EXPECT_EQ(secondsUntilFetch(6 * 3600, AT_0613, false, 2), 13 * 60);
}
