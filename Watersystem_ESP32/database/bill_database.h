#ifndef BILL_DATABASE_H
#define BILL_DATABASE_H

#include "../configuration/config.h"

// ===== BILL DATA STRUCTURE =====
struct BillData {
  String customerName;
  String accountNo;
  String address;
  String collector;
  String dueDate;
  unsigned long prevReading;
  unsigned long currReading;
  float rate;
  float penalty;
};

// Current bill (sample data)
BillData currentBill = {
  "Juan Dela Cruz",
  "WSM-000123",
  "Purok 2, Makilas",
  "A. MACASLING",
  "Jan 15, 2026",
  12345,
  12357,
  15.00,
  0.00
};

// ===== WATER SYSTEM FUNCTIONS =====

// Calculate water consumption
unsigned long getWaterUsage() {
  return currentBill.currReading - currentBill.prevReading;
}

// Calculate total amount due
float calculateTotalDue() {
  unsigned long usage = getWaterUsage();
  float billAmount = usage * currentBill.rate;
  return billAmount + currentBill.penalty;
}

// Update meter readings
void updateReadings(unsigned long newReading) {
  currentBill.prevReading = currentBill.currReading;
  currentBill.currReading = newReading;
  Serial.print(F("Reading updated: "));
  Serial.println(newReading);
}

// Set customer data
void setCustomerData(String name, String account, String address, float rate) {
  currentBill.customerName = name;
  currentBill.accountNo = account;
  currentBill.address = address;
  currentBill.rate = rate;
  Serial.println(F("Customer data updated"));
}

// Get bill summary as string
String getBillSummary() {
  String summary = "";
  summary += "Customer: " + currentBill.customerName + "\n";
  summary += "Account: " + currentBill.accountNo + "\n";
  summary += "Address: " + currentBill.address + "\n";
  summary += "Usage: " + String(getWaterUsage()) + " cu.m\n";
  summary += "Rate: P" + String(currentBill.rate, 2) + "/cu.m\n";
  summary += "Amount: P" + String(calculateTotalDue(), 2) + "\n";
  return summary;
}

#endif  // BILL_DATABASE_H
