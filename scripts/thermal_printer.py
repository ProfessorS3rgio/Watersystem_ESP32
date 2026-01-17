import win32print
from datetime import datetime
from barcode import UPCA
from escpos.printer import Dummy
from PIL import Image

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
    
    def print_bill(self, bill_data):
        """Print a full water bill with all details"""
        try:
            hPrinter = win32print.OpenPrinter(self.printer_name)
            win32print.StartDocPrinter(hPrinter, 1, ("Water Bill", None, "RAW"))
            
            bill_content = self._format_water_bill(bill_data)
            
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
        # Use full account number (CODE39 supports A-Z, 0-9, and symbols)
        # No filtering needed since CODE39 can encode letters
        
        # Use amount as integer (no cents conversion)
        amount_int = int(amount)
        
        # Create barcode data: account + amount (no separators needed for CODE39)
        barcode_data = f"{account_no}{amount_int}"
        
        # Use python-escpos to generate barcode commands
        dummy_printer = Dummy()
        
        # Generate CODE39 barcode without text below
        dummy_printer.barcode(barcode_data, 'CODE39', width=2, height=100, pos='OFF', font='A')
        
        # Get the ESC/POS commands
        barcode_commands = dummy_printer.output
        
        return barcode_commands
    
    def _print_image(self, image_path):
        """Generate ESC/POS commands to print a BMP/PNG image"""
        try:
            # Load image with PIL
            img = Image.open(image_path).convert('1')  # Convert to 1-bit monochrome
            
            # Use python-escpos to generate image commands
            dummy_printer = Dummy()
            dummy_printer.image(img)
            
            return dummy_printer.output
        except Exception as e:
            print(f"Error loading image {image_path}: {e}")
            return b''
    
    def _format_water_bill(self, bill_data):
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
            line
        )
        
        # Bill title
        title = (
            self.LEFT + self.BOLD_ON + b'WATER BILL\n' + self.BOLD_OFF +
            f'Bill No: {bill_data.get("bill_no", "N/A")}\n'.encode() +
            f'Date   : {datetime.now().strftime("%Y-%m-%d")}\n'.encode() +
            line
        )
        
        # Customer info
        customer_info = (
            f'Customer : {bill_data["customer_name"]}\n'.encode() +
            f'Account  : {bill_data["account_no"]}\n'.encode() +
            f'Address  : {bill_data.get("address", "N/A")}\n'.encode() +
            line
        )
        
        # Collector and penalty
        collector_info = (
            f'Collector: {bill_data.get("collector", "N/A")}\n'.encode() +
            f'Penalty  : PHP {bill_data.get("penalty", 0):.2f}\n'.encode() +
            f'Due Date : {bill_data.get("due_date", "N/A")}\n'.encode() +
            line
        )
        
        # Meter readings section
        readings_section = (
            self.BOLD_ON + b'METER READINGS\n' + self.BOLD_OFF +
            f'Previous : {bill_data["prev_reading"]}\n'.encode() +
            f'Current  : {bill_data["curr_reading"]}\n'.encode() +
            f'Used     : {used} m3\n'.encode() +
            line
        )
        
        # Rate and total
        calculation_section = (
            f'Rate/m3  : PHP {bill_data["rate"]:.2f}\n'.encode() +
            double_line +
            self.CENTER + self.BOLD_ON + b'*** TOTAL AMOUNT DUE ***\n' + self.BOLD_OFF +
            f'PHP {total:.2f}\n'.encode() +
            double_line
        )
        
        # Footer message
        footer = (
            self.CENTER + b'\nPlease pay on or before due date\nto avoid penalties.\n\nThank you!\n' +
            self.LEFT
        )
        
        # Generate CODE39 barcode with account number and amount
        barcode_commands = self._generate_barcode_data(bill_data["account_no"], total)
        
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
        
        # Combine all sections (no automatic cut)
        return (header + title + customer_info + collector_info + 
                readings_section + calculation_section + footer + 
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