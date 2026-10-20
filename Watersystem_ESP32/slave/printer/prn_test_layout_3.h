#ifndef PRN_TEST_LAYOUT_3_H
#define PRN_TEST_LAYOUT_3_H

#include "prn_test.h"

// Layout 3: continuous CP437 box-drawing table. The printer's self-test
// confirms CP437 support, whose line glyphs join unlike ordinary | and -.
// All rows remain 32 characters wide for the 58 mm QR204 and Font B.
enum PrnTestLayout3Cp437 : uint8_t {
  PRN3_VERTICAL = 0xB3,
  PRN3_HORIZONTAL = 0xC4,
  PRN3_TOP_LEFT = 0xDA,
  PRN3_TOP_TEE = 0xC2,
  PRN3_TOP_RIGHT = 0xBF,
  PRN3_LEFT_TEE = 0xC3,
  PRN3_CROSS = 0xC5,
  PRN3_RIGHT_TEE = 0xB4,
  PRN3_BOTTOM_LEFT = 0xC0,
  PRN3_BOTTOM_TEE = 0xC1,
  PRN3_BOTTOM_RIGHT = 0xD9
};

static void printPrnTestLayout3Repeated(uint8_t character, uint8_t count) {
  while (count-- > 0) printer.write(character);
}

static String prnTestLayout3Fit(const String& value, size_t width) {
  return value.length() <= width ? value : value.substring(0, width);
}

static void printPrnTestLayout3FullTopBorder() {
  printer.write(PRN3_TOP_LEFT);
  printPrnTestLayout3Repeated(PRN3_HORIZONTAL, 30);
  printer.write(PRN3_TOP_RIGHT);
  printer.write('\n');
}

static void printPrnTestLayout3SplitBorder() {
  printer.write(PRN3_LEFT_TEE);
  printPrnTestLayout3Repeated(PRN3_HORIZONTAL, 10);
  printer.write(PRN3_TOP_TEE);
  printPrnTestLayout3Repeated(PRN3_HORIZONTAL, 19);
  printer.write(PRN3_RIGHT_TEE);
  printer.write('\n');
}

static void printPrnTestLayout3MiddleBorder() {
  printer.write(PRN3_LEFT_TEE);
  printPrnTestLayout3Repeated(PRN3_HORIZONTAL, 10);
  printer.write(PRN3_CROSS);
  printPrnTestLayout3Repeated(PRN3_HORIZONTAL, 19);
  printer.write(PRN3_RIGHT_TEE);
  printer.write('\n');
}

static void printPrnTestLayout3BottomBorder() {
  printer.write(PRN3_BOTTOM_LEFT);
  printPrnTestLayout3Repeated(PRN3_HORIZONTAL, 10);
  printer.write(PRN3_BOTTOM_TEE);
  printPrnTestLayout3Repeated(PRN3_HORIZONTAL, 19);
  printer.write(PRN3_BOTTOM_RIGHT);
  printer.write('\n');
}

static void printPrnTestLayout3Row(const String& label, const String& value, bool lastRow = false) {
  const String left = prnTestLayout3Fit(label, 10);
  const String right = prnTestLayout3Fit(value, 19);
  printer.write(PRN3_VERTICAL);
  printer.print(left);
  for (size_t space = left.length(); space < 10; ++space) {
    printer.print(' ');
  }
  printer.write(PRN3_VERTICAL);
  printer.print(right);
  for (size_t space = right.length(); space < 19; ++space) {
    printer.print(' ');
  }
  printer.write(PRN3_VERTICAL);
  printer.write('\n');
  if (lastRow) {
    printPrnTestLayout3BottomBorder();
  } else {
    printPrnTestLayout3MiddleBorder();
  }
}

static void printPrnTestLayout3Section(const String& title) {
  const String text = prnTestLayout3Fit(title, 30);
  const size_t leftPadding = (30 - text.length()) / 2;
  printer.boldOn();
  printer.write(PRN3_VERTICAL);
  for (size_t space = 0; space < leftPadding; ++space) printer.print(' ');
  printer.print(text);
  for (size_t space = leftPadding + text.length(); space < 30; ++space) printer.print(' ');
  printer.write(PRN3_VERTICAL);
  printer.write('\n');
  printer.boldOff();
  printPrnTestLayout3SplitBorder();
}

// Experimental form-style layout. Make changes here only; Layouts 1 and 2
// and the production bill formatter are intentionally independent.
inline void printPrnTestLayout3() {
  loadPrnTestData();
  printPrnTestHeading(true);

  const unsigned long used = currentBill.currReading - currentBill.prevReading;
  const float total = currentBill.total;

  printer.justify('L');
  // Font B is 9x17 according to this printer's self-test. Matching the line
  // spacing lets adjacent CP437 vertical glyphs meet with no blank gap.
  printer.setLineSpacing(17);
  printPrnTestLayout3FullTopBorder();
  printPrnTestLayout3Section("BILL INFORMATION");
  printPrnTestLayout3Row("Ref No", currentBill.refNumber);
  printPrnTestLayout3Row("Date", formatDateTime12Hour(currentBill.readingDateTime));

  printPrnTestLayout3Section("CUSTOMER INFORMATION");
  printPrnTestLayout3Row("Customer", prnTestFitText(currentBill.customerName));
  printPrnTestLayout3Row("Account", currentBill.accountNo);
  printPrnTestLayout3Row("Class", currentBill.customerType);
  printPrnTestLayout3Row("Address", prnTestFitText(currentBill.address));
  printPrnTestLayout3Row("Barangay", "Dona Josefa");
  printPrnTestLayout3Row("Reader", currentBill.collector);

  printPrnTestLayout3Section("METER READINGS");
  printPrnTestLayout3Row("Previous", String(currentBill.prevReading));
  printPrnTestLayout3Row("Present", String(currentBill.currReading));
  printPrnTestLayout3Row("Usage", String(used) + " m3");

  printPrnTestLayout3Section("BILLING SUMMARY");
  printPrnTestLayout3Row("Rate/m3", String("PHP ") + String(currentBill.rate, 2));
  printPrnTestLayout3Row("Water Bill", String("PHP ") + String(currentBill.subtotal, 2));
  if (currentBill.deductions > 0) {
    printPrnTestLayout3Row("Deduction", String("PHP -") + String(currentBill.deductions, 2));
  }
  if (currentBill.penalty > 0) {
    printPrnTestLayout3Row("Penalty", String("PHP ") + String(currentBill.penalty, 2));
  }

  printer.boldOn();
  printPrnTestLayout3Row("TOTAL", String("PHP ") + String(total, 2));
  printer.boldOff();

  printPrnTestLayout3Section("PAYMENT SCHEDULE");
  printPrnTestLayout3Row("Due Date", calculateDueDate(currentBill.billDate, getBillDueDaysSetting()));
  printPrnTestLayout3Row("Disconnect", calculateDueDate(currentBill.billDate, getDisconnectionDaysSetting()));
  printPrnTestLayout3Row("After Due", String("PHP ") + String(total + prnTestAfterDueSurcharge(currentBill), 2), true);

  printer.setLineSpacing(30);

  printer.justify('C');
  printer.println();
  printer.println(F("Thank you!"));
  printer.println(F("Save Water, Save Life!"));
  printer.justify('L');
  for (int line = 0; line < 3; ++line) printer.write('\n');
  vTaskDelay(pdMS_TO_TICKS(500));
  printer.setFont('A');
}

#endif  // PRN_TEST_LAYOUT_3_H
