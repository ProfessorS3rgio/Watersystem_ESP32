# Water Billing System - Workflow Guide

## Workflow States and Flow

### 1. **WELCOME STATE** (STATE_WELCOME)
- Initial screen showing organization info
- Press **D** or **B** to start the billing process
- Can also use serial command: `START`

### 2. **ENTER ACCOUNT STATE** (STATE_ENTER_ACCOUNT)
- Display: "ENTER ACCOUNT #"
- **0-9**: Add digit to account number (shown on display)
- **D**: Confirm and search for account
  - If found → Go to STATE_ACCOUNT_FOUND
  - If not found → Show error, stay in ENTER_ACCOUNT
- **C**: Clear the input buffer

### 3. **ACCOUNT FOUND STATE** (STATE_ACCOUNT_FOUND)
- Display: Customer info
  - Account Number
  - Customer Name
  - Address
  - Previous Reading
- **D**: Proceed to enter reading (go to STATE_ENTER_READING)
- **C**: Cancel and return to welcome screen

### 4. **ENTER READING STATE** (STATE_ENTER_READING)
- Display: "CURRENT READING" with customer name
- **0-9**: Add digit to meter reading (shown on display)
- **D**: Confirm reading and calculate bill
  - Validates reading > previous reading
  - If valid → Go to STATE_BILL_CALCULATED
  - If invalid → Show error, stay in ENTER_READING
- **C**: Clear the input buffer

### 5. **BILL CALCULATED STATE** (STATE_BILL_CALCULATED)
- Display: Bill summary showing:
  - Customer Name
  - Water Used (m³)
  - Rate (PHP/m³)
  - **TOTAL DUE** (large display)
- **D**: Print the bill (then return to WELCOME)
- **C**: Cancel and return to welcome screen

---

## Serial Commands

| Command | Action |
|---------|--------|
| `P` | Print sample bill (Juan Dela Cruz) |
| `D` | Display sample bill on TFT |
| `S` | Check SD card status |
| `L` | List all customers in database |
| `START` | Start the account entry workflow |

---

## Example Usage

### Via Keypad:
```
1. Press D on keypad
2. Press 0, 0, 1 for account 001
3. Press D to confirm (Theodore Romero's account appears)
4. Press D to continue to reading entry
5. Press 1, 2, 6, 0 for reading 1260
6. Press D to calculate bill
7. Press D to print the water bill
```

### Via Serial:
```
1. Send "START" command
2. Send "001" (enters digits in buffer)
3. Send "D" (confirms account)
4. (Continue as above)
```

---

## Customer Database

Three customers pre-loaded in memory:

| Account | Name | Address | Previous Reading |
|---------|------|---------|------------------|
| 001 | Theodore Romero | Purok 2 Makilas | 1250 |
| 002 | Billy Mamaril | Purok 2 Makilas | 1220 |
| 003 | John Doe | Purok 2 Makilas | 1210 |

Add more customers using: `addCustomer("004", "Name", "Address", 1200);`

---

## Key Features

✅ **Keypad Input**: Full numeric and control key support  
✅ **Account Lookup**: Quick search in customer database  
✅ **Real-time Calculation**: Bill calculates on the spot  
✅ **TFT Display**: Shows status and prompts at each step  
✅ **Thermal Printer**: Prints formatted bill receipt  
✅ **Error Handling**: Validates account exists and reading is valid  
✅ **Flexible Input**: Accept input via keypad OR serial commands  

---

## File Dependencies

- `workflow_manager.h` - State machine and display functions
- `keypad_manager.h` - Keypad input handling and routing
- `customers_database.h` - Customer data and lookup
- `water_system.h` - Bill calculation logic
- `config.h` - Pin definitions and constants
- `sdcard_manager.h` - SD card operations
- `logo.h` - Thermal printer logo bitmap
