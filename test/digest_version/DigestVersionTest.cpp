#include <gtest/gtest.h>

#include "DigestVersion.h"

using DigestVersion::isNewerRevision;
using DigestVersion::revision;

TEST(DigestVersionTest, ReadsTheRevision) {
  EXPECT_EQ(revision("1.6.5-digest.3"), 3);
  EXPECT_EQ(revision("1.6.5-digest.12"), 12);
}

TEST(DigestVersionTest, PlainDigestBuildIsRevisionZero) { EXPECT_EQ(revision("1.6.5-digest"), 0); }

TEST(DigestVersionTest, StockBuildIsNotADigestBuild) {
  EXPECT_EQ(revision("1.6.5"), -1);
  EXPECT_EQ(revision("1.6.5-rc+abc123"), -1);
}

TEST(DigestVersionTest, LaterRevisionIsNewer) {
  EXPECT_TRUE(isNewerRevision("1.6.5-digest.4", "1.6.5-digest.3"));
  EXPECT_TRUE(isNewerRevision("1.6.5-digest.1", "1.6.5-digest"));
  EXPECT_TRUE(isNewerRevision("1.6.5-digest.10", "1.6.5-digest.9"));
}

TEST(DigestVersionTest, SameOrEarlierRevisionIsNotNewer) {
  EXPECT_FALSE(isNewerRevision("1.6.5-digest.3", "1.6.5-digest.3"));
  EXPECT_FALSE(isNewerRevision("1.6.5-digest.2", "1.6.5-digest.3"));
  EXPECT_FALSE(isNewerRevision("1.6.5-digest", "1.6.5-digest.1"));
}

TEST(DigestVersionTest, StockReleaseNeverCountsAsANewerRevision) {
  EXPECT_FALSE(isNewerRevision("1.6.5", "1.6.5-digest.3"));
  EXPECT_FALSE(isNewerRevision("1.6.5-digest.3", "1.6.5"));
}
