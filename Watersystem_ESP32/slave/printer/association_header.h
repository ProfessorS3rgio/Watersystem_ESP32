#ifndef ASSOCIATION_HEADER_H
#define ASSOCIATION_HEADER_H

#include "printer_serial.h"

// Shared no-logo header for bills, receipts, and printer-layout tests.
// The QR204 text table is CP437-compatible; byte 0xA4 prints the n-with-tilde
// in "Doña" (UTF-8 characters would print incorrectly).
extern ThermalPrinter printer;

inline void printAssociationHeader(const __FlashStringHelper* documentTitle,
                                   bool keepFontB = false,
                                   char titleSize = 'S') {
  printer.justify('C');
  printer.setFont('B');
  printer.boldOn();
  printer.print(F("Do"));
  printer.write(0xA4);
  printer.println(F("a Josefa Makilas Buluan Caparan"));
  printer.println(F("Water System Association"));
  printer.boldOff();
  printer.println(F("Makilas Ipil Zamboanga Sibugay"));
  printer.println(F("TIN: 464-252-005"));

  if (!keepFontB) {
    printer.setFont('A');
  }
  printer.justify('L');
  printer.println();
  printer.justify('C');
  printer.setSize(titleSize);
  printer.boldOn();
  printer.println(documentTitle);
  printer.boldOff();
  printer.setSize('S');
  printer.justify('L');
  printer.println();
}

#endif  // ASSOCIATION_HEADER_H
