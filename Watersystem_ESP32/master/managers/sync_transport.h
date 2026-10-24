#ifndef DMBC_SYNC_TRANSPORT_H
#define DMBC_SYNC_TRANSPORT_H
#include <Arduino.h>

// Protocol responses use the current command's transport; diagnostic logs stay USB-only.
static Print* g_syncResponseOutput = nullptr;
static Print& syncOutput() { return g_syncResponseOutput ? *g_syncResponseOutput : Serial; }
static bool syncOverWifi() { return g_syncResponseOutput != nullptr; }

class SyncOutputScope {
  Print* previous;
public:
  explicit SyncOutputScope(Print& output) : previous(g_syncResponseOutput) { g_syncResponseOutput = &output; }
  ~SyncOutputScope() { g_syncResponseOutput = previous; }
};
#endif
