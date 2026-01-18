#ifndef SYNC_UTILS_H
#define SYNC_UTILS_H

#include <Arduino.h>

inline String sanitizeSyncField(const String& s) {
  String out = s;
  out.replace("\r", " ");
  out.replace("\n", " ");
  out.replace("|", " ");
  return out;
}

#endif // SYNC_UTILS_H