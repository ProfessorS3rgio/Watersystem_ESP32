#ifndef BILL_DATABASE_H
#define BILL_DATABASE_H

#include <Arduino.h>  // for String

struct BillData {
    String refNumber;
    String readingDateTime;
    String customerName;
    String accountNo;
    String customerType;
    String address;
    String collector;
    unsigned long prevReading;
    unsigned long currReading;
    float rate;
    float subtotal;
    float deductions;
    String deductionName;          // text label for deduction, used by printer
    float penalty;
    float total;
    String billDate;
};

extern BillData currentBill;

// stub settings functions used by printer code
int getBillDueDaysSetting();
int getDisconnectionDaysSetting();

#endif // BILL_DATABASE_H
