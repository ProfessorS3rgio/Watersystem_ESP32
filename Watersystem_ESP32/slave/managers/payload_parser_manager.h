#ifndef PAYLOAD_PARSER_MANAGER_H
#define PAYLOAD_PARSER_MANAGER_H

#include <Arduino.h>
#include "../database/bill_database.h"
#include "../database/bill_transactions_database.h"

extern BillData currentBill;
extern ReceiptData currentReceipt;
extern int g_billDueDayOfMonth;
extern int g_disconnectionDayOfMonth;

static void splitString(const String &src, char delim, String dest[], int &count, int maxCount) {
    count = 0;
    int start = 0;
    while (count < maxCount) {
        int idx = src.indexOf(delim, start);
        if (idx == -1) {
            dest[count++] = src.substring(start);
            break;
        }
        dest[count++] = src.substring(start, idx);
        start = idx + 1;
    }
}

void parseBillPayload(const String &payload) {
    String p[20];
    int c;
    splitString(payload, '|', p, c, 20);
    if (c < 11) return; // not enough fields
    // New payloads insert barangay between address and collector. Keep old
    // payloads working so master and slave firmware can be updated separately.
    const int barangayOffset = (c >= 19) ? 1 : 0;
    currentBill.refNumber      = p[0];
    currentBill.readingDateTime = p[1];
    currentBill.customerName   = p[2];
    currentBill.accountNo      = p[3];
    currentBill.customerType   = p[4];
    currentBill.address        = p[5];
    currentBill.barangay       = barangayOffset ? p[6] : "";
    currentBill.collector      = p[6 + barangayOffset];
    currentBill.prevReading    = p[7 + barangayOffset].toInt();
    currentBill.currReading    = p[8 + barangayOffset].toInt();
    currentBill.rate           = p[9 + barangayOffset].toFloat();
    currentBill.subtotal       = p[10 + barangayOffset].toFloat();

    // Default values
    currentBill.deductions = 0;
    currentBill.deductionName = "";
    currentBill.penalty = 0;
    currentBill.total = currentBill.subtotal;
    currentBill.billDate = "";
    g_billDueDayOfMonth = 5;
    g_disconnectionDayOfMonth = 8;

    // ...|rate|subtotal|deductionName|deductions|penalty|total|billDate|dueDay|disconnectDay
    if (c >= 18 + barangayOffset) {
        currentBill.deductionName = p[11 + barangayOffset];
        currentBill.deductions = p[12 + barangayOffset].toFloat();
        currentBill.penalty = p[13 + barangayOffset].toFloat();
        currentBill.total = p[14 + barangayOffset].toFloat();
        currentBill.billDate = p[15 + barangayOffset];

        int dueDay = p[16 + barangayOffset].toInt();
        int disDay = p[17 + barangayOffset].toInt();
        if (dueDay >= 1 && dueDay <= 31) g_billDueDayOfMonth = dueDay;
        if (disDay >= 1 && disDay <= 31) g_disconnectionDayOfMonth = disDay;
        return;
    }

    // Preferred payload from master:
    // ...|rate|subtotal|deductions|penalty|total|billDate|dueDay|disconnectDay
    if (c >= 17 + barangayOffset) {
        currentBill.deductions = p[11 + barangayOffset].toFloat();
        currentBill.penalty = p[12 + barangayOffset].toFloat();
        currentBill.total = p[13 + barangayOffset].toFloat();
        currentBill.billDate = p[14 + barangayOffset];

        int dueDay = p[15 + barangayOffset].toInt();
        int disDay = p[16 + barangayOffset].toInt();
        if (dueDay >= 1 && dueDay <= 31) g_billDueDayOfMonth = dueDay;
        if (disDay >= 1 && disDay <= 31) g_disconnectionDayOfMonth = disDay;
        return;
    }

    // Legacy/extended payload:
    // ...|rate|subtotal|deductions|penalty|total|billDate
    if (c >= 15 + barangayOffset) {
        currentBill.deductions = p[11 + barangayOffset].toFloat();
        currentBill.penalty = p[12 + barangayOffset].toFloat();
        currentBill.total = p[13 + barangayOffset].toFloat();
        currentBill.billDate = p[14 + barangayOffset];
        return;
    }

    // Compact payload from master (current):
    // ...|rate|subtotal|total|billDate
    if (c >= 13 + barangayOffset) {
        currentBill.total = p[11 + barangayOffset].toFloat();
        currentBill.billDate = p[12 + barangayOffset];
        return;
    }

    if (c > 11 + barangayOffset) currentBill.deductions = p[11 + barangayOffset].toFloat();
    if (c > 12 + barangayOffset) currentBill.penalty = p[12 + barangayOffset].toFloat();
    currentBill.total = currentBill.subtotal - currentBill.deductions + currentBill.penalty;
}

void parseReceiptPayload(const String &payload) {
    String p[25];
    int c;
    splitString(payload, '|', p, c, 25);
    if (c < 13) return;
    const int barangayOffset = (c >= 18) ? 1 : 0;
    currentReceipt.receiptNumber   = p[0];
    currentReceipt.paymentDateTime = p[1];
    currentReceipt.billDate        = p[2];
    currentReceipt.customerName    = p[3];
    currentReceipt.accountNo       = p[4];
    currentReceipt.customerType    = p[5];
    currentReceipt.address         = p[6];
    currentReceipt.barangay        = barangayOffset ? p[7] : "";
    currentReceipt.collector       = p[7 + barangayOffset];
    currentReceipt.prevReading     = p[8 + barangayOffset].toInt();
    currentReceipt.currReading     = p[9 + barangayOffset].toInt();
    currentReceipt.rate            = p[10 + barangayOffset].toFloat();
    currentReceipt.subtotal        = p[11 + barangayOffset].toFloat();
    currentReceipt.deductions      = p[12 + barangayOffset].toFloat();
    currentReceipt.penalty         = p[13 + barangayOffset].toFloat();
    currentReceipt.total           = (c > 14 + barangayOffset ? p[14 + barangayOffset].toFloat()
                                          : currentReceipt.subtotal - currentReceipt.deductions + currentReceipt.penalty);
    currentReceipt.amountPaid      = (c > 15 + barangayOffset ? p[15 + barangayOffset].toFloat() : currentReceipt.total);
    currentReceipt.change          = (c > 16 + barangayOffset ? p[16 + barangayOffset].toFloat() : 0);
}

#endif // PAYLOAD_PARSER_MANAGER_H
