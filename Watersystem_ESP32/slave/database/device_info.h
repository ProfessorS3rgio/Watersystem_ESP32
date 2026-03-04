#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <Arduino.h>

// minimal stub to satisfy references in bill_printer.h
static String getDeviceInfoValue(const String &key) {
    // return empty string for any query
    return String();
}

#endif // DEVICE_INFO_H
