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
    currentBill.refNumber      = p[0];
    currentBill.readingDateTime = p[1];
    currentBill.customerName   = p[2];
    currentBill.accountNo      = p[3];
    currentBill.customerType   = p[4];
    currentBill.address        = p[5];
    currentBill.collector      = p[6];
    currentBill.prevReading    = p[7].toInt();
    currentBill.currReading    = p[8].toInt();
    currentBill.rate           = p[9].toFloat();
    currentBill.subtotal       = p[10].toFloat();

    // Default values
    currentBill.deductions = 0;
    currentBill.penalty = 0;
    currentBill.total = currentBill.subtotal;
    currentBill.billDate = "";
    g_billDueDayOfMonth = 5;
    g_disconnectionDayOfMonth = 8;

    // Preferred payload from master:
    // ...|rate|subtotal|deductions|penalty|total|billDate|dueDay|disconnectDay
    if (c >= 17) {
        currentBill.deductions = p[11].toFloat();
        currentBill.penalty = p[12].toFloat();
        currentBill.total = p[13].toFloat();
        currentBill.billDate = p[14];

        int dueDay = p[15].toInt();
        int disDay = p[16].toInt();
        if (dueDay >= 1 && dueDay <= 31) g_billDueDayOfMonth = dueDay;
        if (disDay >= 1 && disDay <= 31) g_disconnectionDayOfMonth = disDay;
        return;
    }

    // Legacy/extended payload:
    // ...|rate|subtotal|deductions|penalty|total|billDate
    if (c >= 15) {
        currentBill.deductions = p[11].toFloat();
        currentBill.penalty = p[12].toFloat();
        currentBill.total = p[13].toFloat();
        currentBill.billDate = p[14];
        return;
    }

    // Compact payload from master (current):
    // ...|rate|subtotal|total|billDate
    if (c >= 13) {
        currentBill.total = p[11].toFloat();
        currentBill.billDate = p[12];
        return;
    }

    if (c > 11) currentBill.deductions = p[11].toFloat();
    if (c > 12) currentBill.penalty = p[12].toFloat();
    currentBill.total = currentBill.subtotal - currentBill.deductions + currentBill.penalty;
}

void parseReceiptPayload(const String &payload) {
    String p[25];
    int c;
    splitString(payload, '|', p, c, 25);
    if (c < 13) return;
    currentReceipt.receiptNumber   = p[0];
    currentReceipt.paymentDateTime = p[1];
    currentReceipt.customerName    = p[2];
    currentReceipt.accountNo       = p[3];
    currentReceipt.customerType    = p[4];
    currentReceipt.address         = p[5];
    currentReceipt.collector       = p[6];
    currentReceipt.prevReading     = p[7].toInt();
    currentReceipt.currReading     = p[8].toInt();
    currentReceipt.rate            = p[9].toFloat();
    currentReceipt.subtotal        = p[10].toFloat();
    currentReceipt.deductions      = p[11].toFloat();
    currentReceipt.penalty         = p[12].toFloat();
    currentReceipt.total           = (c > 13 ? p[13].toFloat()
                                          : currentReceipt.subtotal - currentReceipt.deductions + currentReceipt.penalty);
    currentReceipt.amountPaid      = (c > 14 ? p[14].toFloat() : currentReceipt.total);
    currentReceipt.change          = (c > 15 ? p[15].toFloat() : 0);
}

#endif // PAYLOAD_PARSER_MANAGER_H
