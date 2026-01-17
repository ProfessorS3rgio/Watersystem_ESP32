#!/usr/bin/env python3
"""
Test script for printing just the cutting design and barcode section.
"""

from thermal_printer import ThermalPrinter

def test_cutting_and_barcode():
    """Test printing just the cutting design and barcode"""
    printer = ThermalPrinter()

    # Sample data for barcode generation
    account_no = "D001"  # Account number with letter prefix as specified
    total_amount = 3500

    print("Testing cutting design and barcode generation...")
    print(f"Account: {account_no}")
    print(f"Amount: PHP {total_amount:.2f}")

    # Generate CODE39 barcode with account number and amount
    barcode_commands = printer._generate_barcode_data(account_no, total_amount)
    barcode_data = f"{account_no}{int(total_amount)}"
    print(f"Generated CODE39 barcode data: {barcode_data}")
    print(f"Generated ESC/POS barcode commands (length: {len(barcode_commands)} bytes)")

    # Test printing just the cutting and barcode section with motto
    cutting_and_barcode = (
        b'\n' +
        b'8< ------------------------------ \n' +  # ASCII scissors with cutting line first
        b'\n\n\n' +  # More space between cutting line and barcode
        barcode_commands +  # ESC/POS barcode commands second
        b'\n' +  # Space after barcode
        b'Save Water, Save Life!\n' +  # Motto below the barcode
        b'\n'
    )

    try:
        import win32print
        hPrinter = win32print.OpenPrinter(printer.printer_name)
        win32print.StartDocPrinter(hPrinter, 1, ("Cutting Test", None, "RAW"))
        win32print.WritePrinter(hPrinter, cutting_and_barcode)
        win32print.EndDocPrinter(hPrinter)
        win32print.ClosePrinter(hPrinter)
        print("✅ Cutting design and CODE39 barcode printed successfully!")
        print(f"The barcode encodes: {barcode_data} (account + amount, no text displayed below)")
    except Exception as e:
        print(f"❌ Print error: {e}")

if __name__ == "__main__":
    test_cutting_and_barcode()