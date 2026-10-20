#ifndef PRN_TEST_LAYOUT_1_H
#define PRN_TEST_LAYOUT_1_H

#include "prn_test.h"

// Layout 1 is the unmodified production bill format from bill_printer.h,
// supplied with the same sample data as the test layout for comparison.
inline void printPrnTestLayout1() {
  loadPrnTestData();
  printBill();
}

inline void printPrnTestLayout1Header() {
  printPrnHeaderOnly();
}

#endif  // PRN_TEST_LAYOUT_1_H
