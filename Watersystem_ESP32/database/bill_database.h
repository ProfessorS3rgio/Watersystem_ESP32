#ifndef BILL_DATABASE_H
#define BILL_DATABASE_H

#include "../configuration/config.h"
#include "customers_database.h"
#include "customer_type_database.h"
#include "deduction_database.h"
#include "device_info.h"
#include <time.h>

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
