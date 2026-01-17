from flask import Flask, request, jsonify
from flask_cors import CORS
from thermal_printer import ThermalPrinter

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# Initialize printer
printer = ThermalPrinter()

@app.route('/print', methods=['POST'])
def print_endpoint():
    data = request.json
    required_fields = ['bill_no', 'amount_paid', 'cash_received', 'change', 'created_at']
    
    if not all(field in data for field in required_fields):
        return jsonify({'success': False, 'error': 'Missing required fields'}), 400
    
    success = printer.print_receipt(
        bill_no=data['bill_no'],
        customer_name=data.get('customer_name'),
        amount_paid=data['amount_paid'],
        cash_received=data['cash_received'],
        change=data['change'],
        created_at=data['created_at']
    )
    
    if success:
        return jsonify({'success': True, 'message': 'Receipt printed successfully'})
    else:
        return jsonify({'success': False, 'error': 'Failed to print receipt'}), 500

@app.route('/print-bill', methods=['POST'])
def print_bill_endpoint():
    data = request.json
    required_fields = ['customer_name', 'account_no', 'prev_reading', 'curr_reading', 'rate']
    
    if not all(field in data for field in required_fields):
        return jsonify({'success': False, 'error': 'Missing required bill fields'}), 400
    
    success = printer.print_bill(data)
    
    if success:
        return jsonify({'success': True, 'message': 'Bill printed successfully'})
    else:
        return jsonify({'success': False, 'error': 'Failed to print bill'}), 500

@app.route('/printers', methods=['GET'])
def printers_endpoint():
    printer_list = printer.get_available_printers()
    return jsonify({'printers': printer_list})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=3000, debug=True)