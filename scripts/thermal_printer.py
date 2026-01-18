import win32print
from datetime import datetime
from PIL import Image
import importlib
import argparse
import os
import random


def _bytes_to_text(data: bytes) -> str:
    """Convert raw ESC/POS bytes to a readable plain-text preview.

    This strips non-printable control bytes while preserving newlines and
    printable ASCII so you can preview the human-readable receipt content.
    """
    out_chars = []
    i = 0
    L = len(data)
    while i < L:
        b = data[i]
        # ESC (27) and GS (29) introduce control sequences; skip them and their
        # immediate parameters to avoid leaving the command letters (e.g. 'a', 'E')
        if b in (27, 29):
            # skip the ESC/GS byte
            i += 1
            # skip up to 3 following parameter/command bytes (common ESC/GS lengths)
            skip = 0
            while i < L and skip < 3:
                i += 1
                skip += 1
            continue

        # Keep LF, CR and printable ASCII
        if b in (10, 13, 9) or 32 <= b < 127:
            out_chars.append(chr(b))
        # otherwise ignore control bytes
        i += 1

    # Collapse excessive blank lines (more than 2)
    text = ''.join(out_chars)
    lines = text.splitlines()
    cleaned_lines = []
    blank_run = 0
    for ln in lines:
        if ln.strip() == '':
            blank_run += 1
            if blank_run <= 2:
                cleaned_lines.append('')
        else:
            blank_run = 0
            # Apply heuristic fixes to remove leftover control letters
            s = ln
            # Remove a leading 'a' or 'E' that precedes uppercase words or symbols
            if len(s) > 1 and s[0] in ('a', 'E') and s[1].isupper():
                s = s[1:].lstrip()

            # Fix common truncated words
            if s.startswith('TER READINGS') or s.startswith('ETER READINGS'):
                s = 'METER READINGS'
            if s.strip().startswith('ill No'):
                s = s.replace('ill No', 'Bill No')

            # Fix missing 'P' in 'PHP' -> 'HP ' or 'H P '
            if s.strip().startswith('HP ') or s.strip().startswith('H P '):
                s = s.replace('H P ', 'PHP ').replace('\n', '\n')
                if not s.strip().startswith('PHP'):
                    s = 'PHP ' + s.strip()[3:]

            # If a line contains a REF with garbage prefix, extract only REF####
            import re
            m = re.search(r'(REF\d{4,})', s)
            if m and (s.strip().startswith(m.group(1)) is False):
                # replace entire line with the REF token if it's trailing garbage
                s = m.group(1)

            cleaned_lines.append(s)

    return '\n'.join(cleaned_lines) + ('\n' if cleaned_lines and not cleaned_lines[-1].endswith('\n') else '')

class ThermalPrinter:
    def __init__(self, printer_name="POS-58"):
        self.printer_name = printer_name
        # ESC/POS commands
        self.ESC = b'\x1b'
        self.GS = b'\x1d'
        self.CENTER = self.ESC + b'\x61\x01'
        self.LEFT = self.ESC + b'\x61\x00'
        self.BOLD_ON = self.ESC + b'\x45\x01'
        self.BOLD_OFF = self.ESC + b'\x45\x00'
        self.CUT = self.GS + b'\x56\x42\x00'
    
    def print_receipt(self, bill_no, customer_name, amount_paid, cash_received, change, created_at):
        """Print a receipt with the given data"""
        try:
            hPrinter = win32print.OpenPrinter(self.printer_name)
            win32print.StartDocPrinter(hPrinter, 1, ("Receipt", None, "RAW"))
            
            receipt_data = self._format_receipt(bill_no, customer_name, amount_paid, cash_received, change, created_at)
            
            win32print.WritePrinter(hPrinter, receipt_data)
            win32print.EndDocPrinter(hPrinter)
            win32print.ClosePrinter(hPrinter)
            return True
        except Exception as e:
            print(f"Print error: {e}")
            return False
    
    def print_bill(self, bill_data, include_cutting=True):
        """Print a full water bill with all details"""
        try:
            hPrinter = win32print.OpenPrinter(self.printer_name)
            win32print.StartDocPrinter(hPrinter, 1, ("Water Bill", None, "RAW"))
            
            bill_content = self._format_water_bill(bill_data, include_cutting=include_cutting)
            
            win32print.WritePrinter(hPrinter, bill_content)
            win32print.EndDocPrinter(hPrinter)
            win32print.ClosePrinter(hPrinter)
            return True
        except Exception as e:
            print(f"Print error: {e}")
            return False
    
    def _format_receipt(self, bill_no, customer_name, amount_paid, cash_received, change, created_at):
        """Format receipt data with ESC/POS commands"""
        line = b'-' * 32 + b'\n'
        
        # Header
        header = (
            self.CENTER + b'DONA JOSEFA M. BULU-AN CAPARAN\n' +
            b'Water & Sanitation Assoc.\n' +
            b'Bulu-an, IPIL, Zambo. Sibugay\n' +
            b'TIN: 464-252-005-000\n' +
            line
        )
        
        # Receipt info
        receipt_info = (
            self.LEFT + b'RECEIPT\n' +
            f'Bill No : {bill_no}\n'.encode() +
            f'Date    : {self._format_datetime(created_at)}\n'.encode() +
            line
        )
        
        # Customer and amounts
        customer_section = (
            f'Customer      : {customer_name or "N/A"}\n'.encode() +
            line +
            f'Amount Paid   : PHP {float(amount_paid):.2f}\n'.encode() +
            f'Cash Received : PHP {float(cash_received):.2f}\n'.encode() +
            f'Change        : PHP {float(change):.2f}\n'.encode() +
            line
        )
        
        # Footer
        footer = (
            self.CENTER + b'Thank you for your payment!\n' +
            self.CUT
        )
        
        return header + receipt_info + customer_section + footer
    
    def _generate_barcode_data(self, account_no, amount):
        """Generate CODE39 barcode data for account number and amount"""
        # Use full account/reference (CODE39 supports A-Z, 0-9, and symbols)
        # The barcode should encode the provided identifier only (reference number),
        # not the amount — callers should pass the desired barcode string as
        # `account_no` (legacy parameter name kept).
        barcode_data = str(account_no)
        
        # Try to use python-escpos to generate barcode commands first
        try:
            escpos_printer_mod = importlib.import_module('escpos.printer')
            Dummy = getattr(escpos_printer_mod, 'Dummy')
            dummy_printer = Dummy()
            dummy_printer.barcode(barcode_data, 'CODE39', width=2, height=100, pos='OFF', font='A')
            return dummy_printer.output
        except Exception:
            # If escpos or its barcode handling isn't available (common when
            # the installed `barcode` package differs), fall back to emitting
            # raw ESC/POS commands for CODE39. This avoids the dependency on
            # python-escpos internals and works with most ESC/POS-compatible
            # thermal printers.
            try:
                # Set barcode height (GS h n)
                height = 100
                cmd = b''
                cmd += self.GS + b'h' + bytes([height])
                # Set barcode width (GS w n) - module width
                cmd += self.GS + b'w' + bytes([2])
                # Hide HRI (human readable) under barcode (GS H n) -> 0 = off
                cmd += self.GS + b'H' + bytes([0])
                # Print CODE39: GS k m d... NUL with m=4 for CODE39
                cmd += self.GS + b'k' + bytes([4]) + barcode_data.encode('ascii') + b'\x00'
                return cmd
            except Exception as e:
                print(f"Failed to build raw ESC/POS barcode bytes: {e}")
                return b''
    
    def _print_image(self, image_path):
        """Generate ESC/POS commands to print a BMP/PNG image"""
        try:
            # Load image with PIL
            img = Image.open(image_path).convert('1')  # Convert to 1-bit monochrome
            
            # Use python-escpos to generate image commands if available
            try:
                escpos_printer_mod = importlib.import_module('escpos.printer')
                Dummy = getattr(escpos_printer_mod, 'Dummy')
                dummy_printer = Dummy()
                dummy_printer.image(img)
                return dummy_printer.output
            except Exception as e:
                print(f"escpos not available for image printing: {e}")
                return b''
        except Exception as e:
            print(f"Error loading image {image_path}: {e}")
            return b''
    
    def _format_water_bill(self, bill_data, include_cutting=True):
        """Format a complete water bill with all details"""
        line = b'-' * 32 + b'\n'
        double_line = b'=' * 32 + b'\n'
        
        # Calculate values
        used = bill_data['curr_reading'] - bill_data['prev_reading']
        total = used * bill_data['rate'] + bill_data.get('penalty', 0)
        
        # Header with logo placeholder (you can add bitmap later)
        header = (
            self.CENTER + b'[LOGO PLACEHOLDER]\n\n' +
            b'DONA JOSEFA M. BULU-AN CAPARAN\n' +
            b'Water & Sanitation Assoc.\n' +
            b'Bulu-an, IPIL, Zambo. Sibugay\n' +
            b'TIN: 464-252-005-000\n' +
            b'\n' +
            self.CENTER + line +
            self.CENTER + self.BOLD_ON + b'STATEMENT OF ACCOUNT\n' + self.BOLD_OFF +
            self.CENTER + line
        )
        # Keep `title` defined (no visible title requested) to satisfy return concatenation
        title = self.LEFT + line
        
        

        # Additional metadata lines: Ref No, Date/Time, Classification, Period Covered,
        # Due Date and Disconnection Date. For narrow paper (58mm) place long
        # values on the following indented line to avoid overflow.
        ref = bill_data.get("reference_no", "N/A")
        # Short datetime for single-line display: MM/DD/YY HH:MMAM/PM
        dt = datetime.now().strftime("%m/%d/%y %I:%M%p")
        classification = bill_data.get("classification", "N/A")
        period = bill_data.get("period_covered", "N/A")
        due = bill_data.get("due_date", "N/A")
        disc = bill_data.get("disconnection_date", "N/A")

        # For narrow (58mm) paper prefer simple multi-line metadata (no boxes)
        meta = (
            f'Ref No       : {ref}\n'.encode() +
            f'Date/Time    : {dt}\n'.encode() +
            line
        )
        
        # Customer info
        customer_info = (
            f'Customer : {bill_data["customer_name"]}\n'.encode() +
            f'Account  : {bill_data["account_no"]}\n'.encode() +
            f'Classification: {classification}\n'.encode() +
            f'Address  : {bill_data.get("address", "N/A")}\n'.encode() +
            line
        )
        
        # Collector and penalty
        collector_info = (
            f'Collector: {bill_data.get("collector", "N/A")}\n'.encode() +
            f'Penalty  : PHP {bill_data.get("penalty", 0):.2f}\n'.encode() +
            # Due Date moved below total
            line
        )
        
        # Period covered (centered block) placed before meter readings
        period_section = (
            self.CENTER + b'Period Covered\n' +
            self.CENTER + f'{period}\n'.encode() +
            b'\n'
        )

        # Meter readings section formatted in fixed-width centered columns (total width ~32)
        prev_w, present_w, usage_w = 10, 12, 10
        header_cols = f'{"Prev":^{prev_w}}{"Present":^{present_w}}{"Usage":^{usage_w}}\n'
        values_cols = f'{bill_data["prev_reading"]:^{prev_w}}{bill_data["curr_reading"]:^{present_w}}{used:^{usage_w}}\n'
        readings_section = (
            self.CENTER + self.BOLD_ON + b'METER READINGS\n' + self.BOLD_OFF +
            self.CENTER + header_cols.encode() +
            self.CENTER + values_cols.encode() +
            line
        )
        
        # Rate and total
        calculation_section = (
            f'Rate/m3  : PHP {bill_data["rate"]:.2f}\n'.encode() +
            f'Seniors Citizen : PHP -20.00\n'.encode() +
            double_line +
            self.CENTER + self.BOLD_ON + b'*** TOTAL AMOUNT DUE ***\n' + self.BOLD_OFF +
            f'PHP {total:.2f}\n'.encode() +
            double_line
        )

        # Format Due/Disconnect dates as DD/MM/YYYY if possible
        def _fmt_short_date(s):
            try:
                # Accept ISO-like strings 'YYYY-MM-DD' or with time
                ds = s.split('T')[0]
                dtobj = datetime.fromisoformat(ds)
                return dtobj.strftime('%d/%m/%Y')
            except Exception:
                return s

        due_fmt = _fmt_short_date(due) if due else 'N/A'
        disc_fmt = _fmt_short_date(disc) if disc else 'N/A'

        due_block = (
            self.LEFT + f'Due Date     : {due_fmt}\n'.encode() +
            f'Disconnection Date: {disc_fmt}\n'.encode() +
            line
        )
        
        # Reminders under total due
        reminders = (
            self.LEFT + b'\nREMAINDERS:\n' +
            b'- Please pay by the due date to avoid disconnection.\n' +
            b'- Bring this bill when paying.\n' +
            b'- For inquiries, contact the office.\n' +
            line
        )

        # Footer message
        footer = (
            self.CENTER + b'\nPlease pay on or before due date\nto avoid penalties.\n\nThank you!\n' +
            self.LEFT
        )
        
        cutting_and_barcode = b''
        if include_cutting:
            # Generate CODE39 barcode with the reference number only
            barcode_identifier = bill_data.get("reference_no", bill_data.get("account_no"))
            barcode_commands = self._generate_barcode_data(barcode_identifier, total)
            # Cutting design with barcode and motto
            cutting_and_barcode = (
                b'\n' +
                self.CENTER +
                b'8< ------------------------------ \n' +  # ASCII scissors with cutting line first
                b'\n\n\n' +  # More space between cutting line and barcode
                barcode_commands +  # ESC/POS barcode commands second
                b'\n' +  # Space after barcode
                b'Save Water, Save Life!\n' +  # Motto below the barcode
                b'\n\n\n' +  # More space after motto
                self.LEFT
            )
        
        # Combine all sections (no automatic cut). Place due/disconnect after total.
        return (header + title + meta + customer_info + collector_info + 
            period_section + readings_section + calculation_section + due_block + reminders + footer + 
            cutting_and_barcode)
    
    def _format_datetime(self, datetime_str):
        """Format datetime string for receipt"""
        try:
            dt = datetime.fromisoformat(datetime_str.replace("Z", "+00:00"))
            return dt.strftime("%Y-%m-%d %H:%M:%S")
        except:
            return datetime_str
    
    def get_available_printers(self):
        """Get list of available printers"""
        try:
            printers = win32print.EnumPrinters(2)
            return [printer[2] for printer in printers]
        except Exception as e:
            print(f"Error getting printers: {e}")
            return []

    def print_raw(self, data, docname="Document"):
        """Send raw ESC/POS bytes to the configured printer as a single job."""
        try:
            hPrinter = win32print.OpenPrinter(self.printer_name)
            win32print.StartDocPrinter(hPrinter, 1, (docname, None, "RAW"))
            win32print.WritePrinter(hPrinter, data)
            win32print.EndDocPrinter(hPrinter)
            win32print.ClosePrinter(hPrinter)
            return True
        except Exception as e:
            print(f"Print error: {e}")
            return False


def _sample_bill():
    collectors = ['Pedro Santos', 'Maria Lopez', 'Jose Ramos', 'Ana Cruz']
    ref_no = f"REF{random.randint(100000, 999999)}"
    return {
        'bill_no': '000123',
        'customer_name': 'Juan Dela Cruz',
        'account_no': 'M001',
        'reference_no': ref_no,
        'address': 'Makilas, IPIL',
        'barangay': 'Makilas',
        'collector': random.choice(collectors),
        'classification': 'Residential',
        'period_covered': 'Jan 2026 - Feb 2026',
        'disconnection_date': '2026-02-10',
        'penalty': 0,
        'due_date': '2026-02-01',
        'prev_reading': 120,
        'curr_reading': 128,
        'rate': 15.0,
    }


def main():
    parser = argparse.ArgumentParser(description='Thermal printer test harness')
    parser.add_argument('--mode', choices=['dummy', 'print'], default='dummy',
                        help='dummy: write ESC/POS bytes to file; print: send to default printer')
    args = parser.parse_args()

    tp = ThermalPrinter()

    if args.mode == 'dummy':
        bill = _sample_bill()
        # Generate a small receipt and a full bill for inspection
        receipt_bytes = tp._format_receipt(bill.get('bill_no'), bill.get('customer_name'),
                                          bill.get('rate') * (bill['curr_reading'] - bill['prev_reading']),
                                          bill.get('rate') * (bill['curr_reading'] - bill['prev_reading']),
                                          0,
                                          datetime.now().isoformat())

        bill_bytes = tp._format_water_bill(bill)

        # Convert ESC/POS bytes to readable text preview instead of binary
        receipt_text = _bytes_to_text(receipt_bytes)
        bill_text = _bytes_to_text(bill_bytes)
        preview = receipt_text + "\n\n--- FULL BILL ---\n\n" + bill_text

        out_path = os.path.join(os.getcwd(), 'receipt_preview.txt')
        try:
            with open(out_path, 'w', encoding='utf-8') as f:
                f.write(preview)
            print(f'Wrote text preview to: {out_path}')
        except Exception as e:
            print(f'Failed to write preview file: {e}')

    else:  # print mode — send only the full bill to avoid duplication
        bill = _sample_bill()
        success = tp.print_bill(bill, include_cutting=False)
        print('Print sent' if success else 'Print failed')


if __name__ == '__main__':
    main()