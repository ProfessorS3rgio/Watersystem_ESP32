#!/usr/bin/env python3
"""
Test script for printing water bills using the ThermalPrinter class.
This demonstrates how to use the print_bill method with sample data.
"""

from thermal_printer import ThermalPrinter

def test_print_bill():
    """Test printing a sample water bill"""
    printer = ThermalPrinter()

    # Sample bill data (similar to ESP32 currentBill struct)
    bill_data = {
        'bill_no': 'WB-2026-001',
        'customer_name': 'Juan Dela Cruz',
        'account_no': 'ACC001',
        'address': '123 Main St, Barangay Sample, Municipality',
        'prev_reading': 1234,
        'curr_reading': 1456,
        'rate': 15.50,  # PHP per cubic meter
        'penalty': 25.00,
        'due_date': '2026-02-15',
        'collector': 'Maria Santos'
    }

    print("Testing bill printing...")
    print(f"Customer: {bill_data['customer_name']}")
    print(f"Account: {bill_data['account_no']}")
    print(f"Previous Reading: {bill_data['prev_reading']}")
    print(f"Current Reading: {bill_data['curr_reading']}")
    print(f"Rate: PHP {bill_data['rate']:.2f}/m³")
    print(f"Penalty: PHP {bill_data['penalty']:.2f}")

    # Calculate totals
    used = bill_data['curr_reading'] - bill_data['prev_reading']
    total = used * bill_data['rate'] + bill_data['penalty']
    print(f"Usage: {used} m³")
    print(f"Total Due: PHP {total:.2f}")

    print("\nBill will include:")
    print("- Company header and logo placeholder")
    print("- Customer and billing information")
    print("- Meter readings and calculations")
    print("- Clean cutting design (dashed line) for cash payments")
    print("- UPC-A barcode with account number and amount due")

    # Print the bill
    success = printer.print_bill(bill_data)

    if success:
        print("✅ Bill printed successfully!")
        print("\nNote: The printed bill includes a clean cutting design (dashed line) that staff can use")
        print("to separate the UPC-A barcode portion when customers pay in cash.")
    else:
        print("❌ Failed to print bill")

if __name__ == "__main__":
    test_print_bill()