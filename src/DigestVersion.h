#pragma once

#include <cstdlib>
#include <cstring>

// Digest builds are versioned <crosspoint version>-digest.<revision>, where the
// revision counts the digest commits on top of that CrossPoint release. A fix
// to the digest code raises the revision without a new CrossPoint release, so
// the update check compares it once the three version numbers match.
namespace DigestVersion {

// The revision after "-digest.", 0 for a plain "-digest" build, -1 for a
// version that isn't a digest build.
inline long revision(const char* version) {
  const char* digest = strstr(version, "-digest");
  if (digest == nullptr) return -1;
  if (digest[7] != '.') return 0;
  return strtol(digest + 8, nullptr, 10);
}

// True when latest is a later digest revision of the same CrossPoint version as
// current. Callers have already found the three version numbers equal.
inline bool isNewerRevision(const char* latest, const char* current) {
  const long latestRevision = revision(latest);
  const long currentRevision = revision(current);
  return latestRevision >= 0 && currentRevision >= 0 && latestRevision > currentRevision;
}

}  // namespace DigestVersion
