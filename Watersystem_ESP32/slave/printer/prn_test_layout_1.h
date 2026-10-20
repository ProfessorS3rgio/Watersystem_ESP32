#ifndef PRN_TEST_LAYOUT_1_H
#define PRN_TEST_LAYOUT_1_H

#include "prn_test.h"

// Layout 1 keeps the production bill body but uses the same no-logo Font B
// association header as Layout 2 for a direct header comparison.
inline void printPrnTestLayout1() {
  loadPrnTestData();
  printPrnTestHeading(true);
  // The shared test heading leaves Font B selected. Restore Font A so the
  // remainder exactly follows the production bill body's appearance.
  printer.setFont('A');
  printBillContent();
}

inline void printPrnTestLayout1Header() {
  printPrnHeaderOnly();
}

#endif  // PRN_TEST_LAYOUT_1_H
