#ifndef BILL_TRANSACTIONS_DATABASE_H
#define BILL_TRANSACTIONS_DATABASE_H

#include <Arduino.h>  // for String

struct ReceiptData {
    String receiptNumber;
    String paymentDateTime;
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
    float penalty;
    float total;
    float amountPaid;
    float change;
};

extern ReceiptData currentReceipt;

#endif // BILL_TRANSACTIONS_DATABASE_H
