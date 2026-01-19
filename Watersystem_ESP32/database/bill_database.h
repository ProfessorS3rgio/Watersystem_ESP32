#ifndef BILL_DATABASE_H
#define BILL_DATABASE_H

#include "../configuration/config.h"
#include "customers_database.h"
#include "customer_type_database.h"
#include "deduction_database.h"
#include "device_info.h"
#include <time.h>
#include <SD.h>
#include <vector>

const int CURRENT_YEAR = 2026;

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
  float subtotal;
  float deductions;
  float total;
  unsigned long usage;
  String customerType;
  float minCharge;
  unsigned long minM3;
  String deductionName;
  String readingDateTime;
};

// ===== BILL STRUCTURE FOR STORAGE =====
struct Bill {
  int bill_id;
  String reference_number;
  int customer_id;
  int reading_id;
  String bill_no;
  String bill_date;
  float rate_per_m3;
  float charges;
  float penalty;
  float total_due;
  String status;
  String created_at;
  String updated_at;
};

// ===== GLOBAL VARIABLES =====
// Current bill (calculated data)
BillData currentBill = {
  "",
  "",
  "",
  "AURELIO MACASLING",
  "Jan 15, 2026",
  0,
  0,
  15.00,
  0.00,
  0.00,
  0.00,
  0.00,
  0,
  "",
  0.00,
  0,
  "",
  ""
};

// Bills database
std::vector<Bill> bills;
static int nextBillId = 1;
static int refSequence = 1;
const char* BILLS_FILE = "/bills.psv";

// ===== UTILITY FUNCTION =====
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// ===== DATABASE FUNCTIONS =====

void loadBillsFromSD();

void initBillsDatabase() {
  loadBillsFromSD();
}

void loadBillsFromSD() {
  bills.clear();
  File file = SD.open(BILLS_FILE, FILE_READ);
  if (!file) {
    Serial.println(F("Bills file not found, starting empty"));
    return;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    // Parse PSV: bill_id|reference_number|customer_id|reading_id|bill_no|bill_date|rate_per_m3|charges|penalty|total_due|status|created_at|updated_at
    int pos = 0;
    Bill b;
    b.bill_id = line.substring(pos, line.indexOf('|', pos)).toInt(); pos = line.indexOf('|', pos) + 1;
    b.reference_number = line.substring(pos, line.indexOf('|', pos)); pos = line.indexOf('|', pos) + 1;
    b.customer_id = line.substring(pos, line.indexOf('|', pos)).toInt(); pos = line.indexOf('|', pos) + 1;
    b.reading_id = line.substring(pos, line.indexOf('|', pos)).toInt(); pos = line.indexOf('|', pos) + 1;
    b.bill_no = line.substring(pos, line.indexOf('|', pos)); pos = line.indexOf('|', pos) + 1;
    b.bill_date = line.substring(pos, line.indexOf('|', pos)); pos = line.indexOf('|', pos) + 1;
    b.rate_per_m3 = line.substring(pos, line.indexOf('|', pos)).toFloat(); pos = line.indexOf('|', pos) + 1;
    b.charges = line.substring(pos, line.indexOf('|', pos)).toFloat(); pos = line.indexOf('|', pos) + 1;
    b.penalty = line.substring(pos, line.indexOf('|', pos)).toFloat(); pos = line.indexOf('|', pos) + 1;
    b.total_due = line.substring(pos, line.indexOf('|', pos)).toFloat(); pos = line.indexOf('|', pos) + 1;
    b.status = line.substring(pos, line.indexOf('|', pos)); pos = line.indexOf('|', pos) + 1;
    b.created_at = line.substring(pos, line.indexOf('|', pos)); pos = line.indexOf('|', pos) + 1;
    b.updated_at = line.substring(pos);
    bills.push_back(b);
    if (b.bill_id >= nextBillId) nextBillId = b.bill_id + 1;
  }
  file.close();
  Serial.print(F("Loaded "));
  Serial.print(bills.size());
  Serial.println(F(" bills from SD"));
}

void saveBillsToSD() {
  File file = SD.open(BILLS_FILE, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to open bills file for writing"));
    return;
  }
  for (const Bill& b : bills) {
    file.print(b.bill_id); file.print('|');
    file.print(b.reference_number); file.print('|');
    file.print(b.customer_id); file.print('|');
    file.print(b.reading_id); file.print('|');
    file.print(b.bill_no); file.print('|');
    file.print(b.bill_date); file.print('|');
    file.print(b.rate_per_m3); file.print('|');
    file.print(b.charges); file.print('|');
    file.print(b.penalty); file.print('|');
    file.print(b.total_due); file.print('|');
    file.print(b.status); file.print('|');
    file.print(b.created_at); file.print('|');
    file.println(b.updated_at);
  }
  file.close();
  Serial.println(F("Bills saved to SD"));
}

int findBillByAccount(String accountNo) {
  int custId = -1;
  int custIdx = findCustomerByAccount(accountNo);
  if (custIdx != -1) {
    Customer* c = getCustomerAt(custIdx);
    if (c) custId = c->customer_id;
  }
  for (size_t i = 0; i < bills.size(); ++i) {
    if (bills[i].customer_id == custId) return i;
  }
  return -1;
}

void addBill(Bill b) {
  bills.push_back(b);
  saveBillsToSD();
}

void updateBill(int index, Bill b) {
  if (index >= 0 && index < (int)bills.size()) {
    bills[index] = b;
    saveBillsToSD();
  }
}

int getBillCount() {
  return bills.size();
}

Bill* getBillAt(int index) {
  if (index >= 0 && index < bills.size()) {
    return &bills[index];
  }
  return nullptr;
}

// ===== DATE TIME FORMATTING =====

String formatEpochToDateTime(uint32_t epoch) {
  if (epoch == 0) return "01/01/70 12:00 AM"; // default if no sync

  time_t t = (time_t)epoch;
  struct tm *tm = localtime(&t);

  char dateStr[12];
  sprintf(dateStr, "%02d/%02d/%02d", tm->tm_mon + 1, tm->tm_mday, tm->tm_year % 100);

  int hour = tm->tm_hour;
  int min = tm->tm_min;
  char ampm[3] = "AM";
  if (hour >= 12) {
    ampm[0] = 'P';
    if (hour > 12) hour -= 12;
  }
  if (hour == 0) hour = 12;

  char timeStr[9];
  sprintf(timeStr, "%02d:%02d %s", hour, min, ampm);

  return String(dateStr) + " " + String(timeStr);
}

String generateReferenceNumber(String accountNo, int year) {
  // Extract numeric part from accountNo, e.g., "M-001" -> "001"
  int dashIndex = accountNo.indexOf('-');
  String accNum = (dashIndex != -1) ? accountNo.substring(dashIndex + 1) : accountNo;
  // Pad to 3 digits
  while (accNum.length() < 3) accNum = "0" + accNum;
  // Year as string
  String yr = String(year);
  // Sequence, pad to 2 digits
  String seqStr = String(refSequence++);
  while (seqStr.length() < 2) seqStr = "0" + seqStr;
  return "REF" + accNum + yr + seqStr;
}

// ===== WATER SYSTEM FUNCTIONS =====

// Calculate water consumption
unsigned long getWaterUsage() {
  if (currentBill.currReading >= currentBill.prevReading) {
    return currentBill.currReading - currentBill.prevReading;
  }
  return 0;
}

// Calculate bill amount based on customer type
float calculateBillAmount(unsigned long usage, unsigned long customerTypeId) {
  // Find customer type
  int typeIndex = findCustomerTypeById(customerTypeId);
  if (typeIndex == -1) {
    // Fallback to default rate if type not found
    return usage * 15.00;
  }
  
  CustomerType* type = getCustomerTypeAt(typeIndex);
  float rate = type->rate_per_m3;
  unsigned long minM3 = type->min_m3;
  float minCharge = type->min_charge;
  
  // Store values for display
  currentBill.rate = rate;
  currentBill.minM3 = minM3;
  currentBill.minCharge = minCharge;
  currentBill.customerType = type->type_name;
  
  // Calculate bill
  if (usage <= minM3) {
    return minCharge;
  } else {
    return minCharge + (usage - minM3) * rate;
  }
}

// Calculate deductions based on customer's specific deduction_id
float calculateDeductions(float subtotal, unsigned long deductionId) {
  if (deductionId == 0) {
    currentBill.deductionName = "";
    return 0.00;  // No deduction
  }
  
  // Find the specific deduction for this customer
  int deductionIndex = findDeductionById(deductionId);
  if (deductionIndex == -1) {
    currentBill.deductionName = "";
    return 0.00;  // Deduction not found
  }
  
  Deduction* deduction = getDeductionAt(deductionIndex);
  if (!deduction) {
    currentBill.deductionName = "";
    return 0.00;
  }
  
  currentBill.deductionName = deduction->name;
  float totalDeduction = 0.00;
  if (deduction->type == "percentage") {
    totalDeduction = subtotal * (deduction->value / 100.0);
  } else if (deduction->type == "fixed") {
    totalDeduction = deduction->value;
  }
  
  return totalDeduction;
}

// Calculate total amount due
float calculateTotalDue(unsigned long customerTypeId, unsigned long deductionId) {
  unsigned long usage = getWaterUsage();
  currentBill.usage = usage;
  
  float subtotal = calculateBillAmount(usage, customerTypeId);
  currentBill.subtotal = subtotal;
  
  float deductions = calculateDeductions(subtotal, deductionId);
  currentBill.deductions = deductions;
  
  float total = subtotal - deductions + currentBill.penalty;
  currentBill.total = total;
  
  return total;
}

// Generate bill for specific customer
bool generateBillForCustomer(String accountNo, unsigned long currentReading) {
  int customerIndex = findCustomerByAccount(accountNo);
  if (customerIndex == -1) {
    Serial.println(F("Customer not found for bill generation"));
    return false;
  }
  
  Customer* customer = getCustomerAt(customerIndex);
  if (!customer) {
    return false;
  }
  
  // Update bill data
  currentBill.customerName = customer->customer_name;
  currentBill.accountNo = customer->account_no;
  currentBill.address = customer->address;
  currentBill.prevReading = customer->previous_reading;
  currentBill.currReading = currentReading;
  
  // Calculate bill using customer type and deduction
  calculateTotalDue(customer->type_id, customer->deduction_id);
  
  // Set reading date and time from last sync epoch
  currentBill.readingDateTime = formatEpochToDateTime(getLastSyncEpoch());
  
  // Create bill record
  Bill newBill;
  int billIndex = findBillByAccount(accountNo);
  String timestamp = formatEpochToDateTime(getLastSyncEpoch());
  if (billIndex != -1) {
    newBill = bills[billIndex];
    newBill.updated_at = timestamp;
  } else {
    newBill.bill_id = nextBillId++;
    newBill.created_at = timestamp;
    newBill.updated_at = timestamp;
  }
  newBill.reference_number = generateReferenceNumber(accountNo, CURRENT_YEAR);
  newBill.customer_id = customer->customer_id;
  newBill.reading_id = 0; // Placeholder
  newBill.bill_no = newBill.reference_number;
  newBill.bill_date = currentBill.readingDateTime;
  newBill.rate_per_m3 = currentBill.rate;
  newBill.charges = currentBill.subtotal;
  newBill.penalty = currentBill.penalty;
  newBill.total_due = currentBill.total;
  newBill.status = "Pending";
  if (billIndex != -1) {
    updateBill(billIndex, newBill);
  } else {
    addBill(newBill);
  }
  
  Serial.print(F("Generated bill for "));
  Serial.print(customer->customer_name);
  Serial.print(F(": Usage="));
  Serial.print(currentBill.usage);
  Serial.print(F(", Subtotal=P"));
  Serial.print(currentBill.subtotal, 2);
  Serial.print(F(", Deductions=P"));
  Serial.print(currentBill.deductions, 2);
  Serial.print(F(", Total=P"));
  Serial.println(currentBill.total, 2);
  
  return true;
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
  summary += "WATER BILL\n";
  summary += "================\n";
  summary += "Customer: " + currentBill.customerName + "\n";
  summary += "Account: " + currentBill.accountNo + "\n";
  summary += "Address: " + currentBill.address + "\n";
  summary += "Type: " + currentBill.customerType + "\n";
  summary += "Due Date: " + currentBill.dueDate + "\n\n";
  
  summary += "METER READINGS\n";
  summary += "Previous: " + String(currentBill.prevReading) + " cu.m\n";
  summary += "Current: " + String(currentBill.currReading) + " cu.m\n";
  summary += "Usage: " + String(currentBill.usage) + " cu.m\n\n";
  
  summary += "CHARGES\n";
  summary += "Rate: P" + String(currentBill.rate, 2) + "/cu.m\n";
  if (currentBill.minM3 > 0) {
    summary += "Min Usage: " + String(currentBill.minM3) + " cu.m\n";
    summary += "Min Charge: P" + String(currentBill.minCharge, 2) + "\n";
  }
  summary += "Subtotal: P" + String(currentBill.subtotal, 2) + "\n";
  
  if (currentBill.deductions > 0) {
    summary += "Deductions: P" + String(currentBill.deductions, 2) + "\n";
  }
  
  if (currentBill.penalty > 0) {
    summary += "Penalty: P" + String(currentBill.penalty, 2) + "\n";
  }
  
  summary += "TOTAL: P" + String(currentBill.total, 2) + "\n\n";
  summary += "Collector: " + currentBill.collector + "\n";
  
  return summary;
}

#endif  // BILL_DATABASE_H
