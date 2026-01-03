# Water Billing System - Logic Flow Documentation

## 1. System Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    WATERSYSTEM ESP32                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  INPUT DEVICES          PROCESSING              OUTPUT DEVICES  │
│  ──────────────         ──────────              ──────────────  │
│                                                                 │
│  • 4x4 Keypad    →     Workflow State Machine  →  TFT Display  │
│                              ↓                       (160x128)  │
│  • Serial Port   →     Account Lookup              Thermal      │
│                              ↓                      Printer     │
│  • SD Card              Bill Calculation            SD Card     │
│                              ↓                      (database)   │
│                        Data Validation                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Workflow State Machine

### State Diagram

```
                    ┌─────────────────────┐
                    │   STATE_WELCOME     │
                    │  (Welcome Screen)   │
                    └──────────┬──────────┘
                               │
                         D or B pressed
                               │
                               ▼
                    ┌─────────────────────┐
                    │ STATE_ENTER_ACCOUNT │
                    │  (Input account #)  │
                    └──────────┬──────────┘
                               │
                         D pressed (Confirm)
                               │
                    ┌──────────▼──────────┐
                    │  Account Lookup     │
                    │  (Search Database)  │
                    └──────┬──────────┬───┘
                           │          │
                    Found   │          │   Not Found
                           │          │
                           ▼          └──────────┐
                    ┌──────────────┐             │
                    │ ACCOUNT_FOUND│             │
                    │ (Show Info)  │             │
                    └──────┬───────┘             │
                           │                      │
                         D │ (Confirm)            │ (Show Error)
                           │                      │
                           ▼                      │
                    ┌──────────────┐              │
                    │ENTER_READING │◄─────────────┘
                    │ (Input meter)│
                    └──────┬───────┘
                           │
                         D │ (Confirm)
                           │
                    ┌──────▼──────────┐
                    │ Validate Reading│
                    │ (Must be >prev)│
                    └──────┬──────┬───┘
                           │      │
                    Valid   │      │   Invalid
                           │      │
                           ▼      └──────────┐
                    ┌─────────────┐          │
                    │ BILL_CALC   │          │
                    │(Show Total) │          │
                    └──────┬──────┘          │
                           │                 │ (Show Error)
                         D │ (Print)         │
                           │                 │
                           ▼                 │
                    ┌──────────────┐         │
                    │ Print Bill   │         │
                    │ (Thermal)    │         │
                    └──────┬───────┘         │
                           │                 │
                           └─────┬───────────┘
                                 │
                           C pressed (Cancel)
                                 │
                                 ▼
                    ┌─────────────────────┐
                    │   STATE_WELCOME     │
                    │ (Reset & Restart)   │
                    └─────────────────────┘
```

---

## 3. Data Flow Diagram

### Complete Workflow Data Flow

```
USER INPUT (Keypad / Serial)
        │
        ▼
┌──────────────────────────┐
│  handleKeypadInput()     │
│  (Route by state)        │
└──────────────┬───────────┘
               │
        ┌──────┴──────┬──────────┬─────────────┬────────────┐
        │             │          │             │            │
        ▼             ▼          ▼             ▼            ▼
   [0-9]         [D]Confirm  [C]Clear     [A]Print     [B]Display
        │             │          │             │            │
        ▼             ▼          ▼             ▼            ▼
  inputBuffer++   VALIDATE   inputBuffer=  printBill()  displayBill()
                   INPUT      ""
                   │
        ┌──────────┼──────────┐
        ▼          ▼          ▼
   ACCOUNT    READING    BILLS
   LOOKUP     VALIDATION  CALC
        │          │          │
        ▼          ▼          ▼
  customers[] currentBill currentBill
  DATABASE    UPDATED    UPDATED
        │          │          │
        └──────────┴──────────┘
                   │
                   ▼
         ┌──────────────────┐
         │  TFT DISPLAY     │
         │  Update Screen   │
         └──────────────────┘
                   │
                   ▼
         ┌──────────────────┐
         │ THERMAL PRINTER  │
         │ Print Receipt    │
         └──────────────────┘
                   │
                   ▼
         ┌──────────────────┐
         │  SD CARD (LOG)   │
         │ Save Transaction │
         └──────────────────┘
```

---

## 4. Account Entry Logic

### Process: STATE_ENTER_ACCOUNT → STATE_ACCOUNT_FOUND

```
┌─────────────────────────────────────┐
│ STATE_ENTER_ACCOUNT                 │
│ Display: "ENTER ACCOUNT #"          │
└─────────────────────────────────────┘
                │
                ▼
        ┌────────────────┐
        │ Wait for input │
        └────┬───────┬──┬┘
            │        │  │
       [0-9]│      [D]│ [C]
            │        │  │
            ▼        ▼  ▼
      inputBuffer  VALIDATE inputBuffer
      += key        │       = ""
      Display     SEARCH    │
      Updated     customers[] Display
      Input         │       Updated
            ┌─────────────┬──┐
            │             │  │
         FOUND        NOT FOUND
            │             │
            ▼             ▼
      ┌──────────┐  ┌──────────┐
      │ACCOUNT   │  │Show      │
      │_FOUND    │  │Error     │
      │State     │  │Msg: "Not │
      │Display:  │  │Found"    │
      │• Account │  │Stay in   │
      │• Name    │  │ENTER_    │
      │• Address │  │ACCOUNT   │
      │• PrevRead│  │(1s delay)│
      │Press D→  │  └──────────┘
      │READING   │
      └──────────┘
```

**Code Logic:**
```cpp
findCustomerByAccount(inputBuffer) {
  for each customer in array:
    if customer.accountNumber == inputBuffer:
      return customer index
  return -1  // Not found
}
```

---

## 5. Reading Entry Logic

### Process: STATE_ENTER_READING → STATE_BILL_CALCULATED

```
┌─────────────────────────────────────┐
│ STATE_ENTER_READING                 │
│ Display: "CURRENT READING"          │
│ Shows: Customer Name & Previous     │
└─────────────────────────────────────┘
                │
                ▼
        ┌────────────────┐
        │ Wait for input │
        └────┬───────┬──┬┘
            │        │  │
       [0-9]│      [D]│ [C]
            │        │  │
            ▼        ▼  ▼
      inputBuffer  VALIDATE inputBuffer
      += key        │       = ""
      Display     Check:   │
      Updated     NEW >    Display
      Input       PREVIOUS Updated
            ┌─────────────┬──┐
            │             │  │
         VALID      INVALID (<=)
            │             │
            ▼             ▼
      ┌──────────┐  ┌──────────┐
      │CALC      │  │Show      │
      │BILL:     │  │Error     │
      │• usage=  │  │Msg: "Must│
      │  new-old │  │be >prev" │
      │• total=  │  │Stay in   │
      │  usage*  │  │ENTER_    │
      │  rate    │  │READING   │
      │→BILL_    │  │(1s delay)│
      │CALC      │  └──────────┘
      │State     │
      │Display:  │
      │• Customer│
      │• Usage   │
      │• Rate    │
      │• TOTAL   │
      │Press D→  │
      │PRINT     │
      └──────────┘
```

**Code Logic:**
```cpp
if (currentReading > customers[index].previousReading) {
  // VALID
  unsigned long used = currentReading - previousReading;
  float total = used * RATE;
  currentState = STATE_BILL_CALCULATED;
} else {
  // INVALID - Show error & stay in STATE_ENTER_READING
  showError("Must be > previous");
}
```

---

## 6. Bill Calculation Logic

### Process: Compute Water Bill

```
INPUT DATA:
├─ previousReading (from customer DB)
├─ currentReading (entered by user)
├─ rate (15.00 PHP/m³)
└─ penalty (0.00 PHP)

        │
        ▼
┌─────────────────────────────┐
│ Calculate Water Usage       │
│ used = current - previous   │
└─────────────────┬───────────┘
                  │
        ┌─────────▼──────────┐
        │ Ex: 1260 - 1250 = 10 m³
        └─────────┬──────────┘
                  │
                  ▼
┌─────────────────────────────┐
│ Calculate Charges           │
│ charges = used * rate       │
└─────────────────┬───────────┘
                  │
        ┌─────────▼──────────┐
        │ Ex: 10 * 15.00 =   │
        │     150.00 PHP     │
        └─────────┬──────────┘
                  │
                  ▼
┌─────────────────────────────┐
│ Add Penalties               │
│ total = charges + penalty   │
└─────────────────┬───────────┘
                  │
        ┌─────────▼──────────┐
        │ Ex: 150.00 + 0 =   │
        │     150.00 PHP     │
        └─────────┬──────────┘
                  │
                  ▼
        ┌─────────────────┐
        │ UPDATE:         │
        │ currentBill     │
        │ {               │
        │  customerName,  │
        │  accountNo,     │
        │  address,       │
        │  prevReading,   │
        │  currReading,   │
        │  rate,          │
        │  penalty        │
        │ }               │
        └─────────────────┘
```

**Calculation Formula:**
```
TOTAL DUE = (Current Reading - Previous Reading) × Rate/m³ + Penalty
```

---

## 7. Serial Command Processing

### Command Flow

```
SERIAL INPUT BUFFER
        │
        ▼
    ┌─────────────────┐
    │ Read & Trim     │
    │ toUpperCase()   │
    └────┬────────────┘
         │
    ┌────┴────┬────┬────┬────┐
    │          │    │    │    │
    ▼          ▼    ▼    ▼    ▼
   "P"       "D" "S" "L" "START"
    │          │    │    │    │
    ▼          ▼    ▼    ▼    ▼
 PRINT    DISPLAY SD   LIST START
 Bill    Sample   Card  Custs Workflow
    │          │    │    │    │
    └──────────┴────┴────┴────┘
              │
              ▼
        ACTION TAKEN
```

**Commands:**
- **P (Print)**: Print sample bill (Juan Dela Cruz)
- **D (Display)**: Show sample bill on TFT
- **S (Status)**: Check SD card status
- **L (List)**: Print all customers to serial
- **START**: Begin workflow (enter account)

---

## 8. Database Operations

### Customer Database Structure

```
┌─────────────────────────────────────────┐
│ customers[] Array (MAX 50 customers)    │
├─────────────────────────────────────────┤
│ [0] Account: 001                        │
│     Name: Theodore Romero               │
│     Address: Purok 2 Makilas            │
│     PrevReading: 1250                   │
│                                         │
│ [1] Account: 002                        │
│     Name: Billy Mamaril                 │
│     Address: Purok 2 Makilas            │
│     PrevReading: 1220                   │
│                                         │
│ [2] Account: 003                        │
│     Name: John Doe                      │
│     Address: Purok 2 Makilas            │
│     PrevReading: 1210                   │
│                                         │
│ [...] (up to 50 total)                  │
└─────────────────────────────────────────┘
```

### Lookup Operation

```
searchTerm: "001" (accountNumber)
        │
        ▼
┌──────────────────────┐
│ findCustomerByAccount│
│ (account number)     │
└────┬────────────────┘
     │
     ▼
Loop through customers[]:
     │
     ├─ customer[0].accountNumber == "001" ?
     │  └─ YES → Return index 0 ✓
     │
     ├─ customer[1].accountNumber == "001" ?
     │  └─ NO
     │
     └─ ...
     
     If FOUND: return index
     If NOT FOUND: return -1
```

---

## 9. Display Output Logic

### TFT Screen Updates

```
MAIN STATE → DISPLAY FUNCTION → TFT OUTPUT

STATE_WELCOME
    ↓
showWelcomeScreen()
    ├─ Fill screen BLACK
    ├─ Title: "DONA JOSEFA M. BULU-AN CAPARAN"
    ├─ Subtitle: "Water & Sanitation Association"
    ├─ Status: "System Ready"
    └─ Instruction: "Press D or B to start billing"

STATE_ENTER_ACCOUNT
    ↓
displayEnterAccountScreen()
    ├─ Header: "ENTER ACCOUNT #"
    ├─ Large display of inputBuffer (numbers as typed)
    ├─ Instruction: "Press D to confirm"
    └─ Instruction: "Press C to clear"

STATE_ACCOUNT_FOUND
    ↓
displayCustomerInfo()
    ├─ Header: "CUSTOMER INFO"
    ├─ Account: {accountNumber}
    ├─ Name: {customerName}
    ├─ Address: {address}
    ├─ Previous Reading: {prevReading}
    ├─ Instruction: "Press D for reading"
    └─ Instruction: "Press C to cancel"

STATE_ENTER_READING
    ↓
displayEnterReadingScreen()
    ├─ Header: "CURRENT READING"
    ├─ Customer Name: {customerName}
    ├─ Large display of inputBuffer
    ├─ Instruction: "Press D to calc"
    └─ Instruction: "Press C to cancel"

STATE_BILL_CALCULATED
    ↓
displayBillCalculated()
    ├─ Header: "BILL SUMMARY"
    ├─ Name: {customerName}
    ├─ Water Used: {usage} m³
    ├─ Rate: PHP {rate}/m³
    ├─ TOTAL DUE (LARGE): PHP {total}
    └─ Instruction: "Press D to print"
```

---

## 10. Printer Integration

### Thermal Printer Output Flow

```
printBill() function called
        │
        ├─ Wake printer from sleep
        ├─ Set to default settings
        │
        ├─ Custom Feed (spacing)
        │
        ├─ Print Logo Bitmap
        │  └─ 384x192 pixel image from logo.h
        │
        ├─ Print Organization Header
        │  ├─ "DONA JOSEFA M. BULU-AN CAPARAN"
        │  ├─ "Water & Sanitation Assoc."
        │  ├─ Address & TIN
        │  └─ Separator line
        │
        ├─ Print Bill Header
        │  ├─ Bill Number (from millis())
        │  ├─ Date (from __DATE__)
        │  └─ Separator
        │
        ├─ Print Customer Info
        │  ├─ Customer Name
        │  ├─ Account Number
        │  ├─ Address
        │  ├─ Collector Name
        │  ├─ Penalty (if any)
        │  ├─ Due Date
        │  └─ Separator
        │
        ├─ Print Meter Readings
        │  ├─ Previous Reading
        │  ├─ Current Reading
        │  ├─ Water Used (m³)
        │  └─ Separator
        │
        ├─ Print Charges
        │  ├─ Rate per m³
        │  └─ Double separator
        │
        ├─ Print Total (Large & Bold)
        │  └─ "PHP XXXX.XX"
        │
        ├─ Print Footer
        │  ├─ "Please pay on or before due date"
        │  ├─ "to avoid penalties."
        │  └─ "Thank you!"
        │
        └─ Custom Feed & Delay
           └─ 4 line feeds + 500ms delay
```

---

## 11. Error Handling

### Input Validation Flow

```
USER INPUT
        │
        ├─ ACCOUNT NUMBER
        │  ├─ Must be numeric (0-9)
        │  ├─ Length: flexible
        │  └─ Must exist in database
        │     └─ If NOT found: Show "Account NOT FOUND"
        │
        └─ METER READING
           ├─ Must be numeric (0-9)
           ├─ Must be INTEGER
           └─ Must be GREATER THAN previous reading
              └─ If <=: Show "Invalid reading! Must be > previous"

ON ERROR:
    1. Display error message on TFT (RED text)
    2. Log to serial: error description
    3. Wait 2 seconds (delay)
    4. Redisplay input screen
    5. Clear inputBuffer
    6. Continue accepting input
```

---

## 12. Global Variables & State

### Key Global Variables

```cpp
// Workflow State
WorkflowState currentState = STATE_WELCOME;

// Input Buffer
String inputBuffer = "";          // Accumulates numeric input

// Selected Customer
int selectedCustomerIndex = -1;   // Index into customers[] array

// Current Reading
unsigned long currentReading = 0; // Latest meter reading

// Customer Database
Customer customers[MAX_CUSTOMERS]; // Array of 50 max
int customerCount = 3;             // Currently loaded

// Bill Data
BillData currentBill;              // Current bill being worked on

// SD Card Status
bool sdCardPresent = false;        // Is SD card inserted?

// Keypad Object
Keypad keypad;                     // 4x4 matrix keypad

// Display Object
Adafruit_ST7735 tft;               // ST7735 TFT display

// Printer Object
Adafruit_Thermal printer;          // Thermal printer
```

---

## 13. Complete Interaction Example

### Real-World Scenario

```
USER: Theodore Romero comes to pay water bill (Account 001)

STEP 1: System is in STATE_WELCOME
        Display: Welcome screen with "Press D or B"
        
STEP 2: USER presses 'D' on keypad
        State → STATE_ENTER_ACCOUNT
        Display: "ENTER ACCOUNT #"
        
STEP 3: USER presses 0, 0, 1
        inputBuffer = "001"
        Display: Shows "001" in large text
        
STEP 4: USER presses 'D' to confirm
        findCustomerByAccount("001") → Returns index 0
        customer found: Theodore Romero
        State → STATE_ACCOUNT_FOUND
        Display: Shows account info
                 Account: 001
                 Name: Theodore Romero
                 Address: Purok 2 Makilas
                 Previous: 1250 m³
        
STEP 5: USER presses 'D' to proceed
        State → STATE_ENTER_READING
        Display: "CURRENT READING"
                 Name: Theodore Romero
        
STEP 6: USER enters meter reading: 1, 2, 6, 0
        inputBuffer = "1260"
        Display: Shows "1260" in large text
        
STEP 7: USER presses 'D' to calculate
        Validate: 1260 > 1250? YES ✓
        Calculate:
          usage = 1260 - 1250 = 10 m³
          total = 10 * 15.00 = 150.00 PHP
        Update currentBill with all data
        State → STATE_BILL_CALCULATED
        Display: Bill Summary
                 Name: Theodore Romero
                 Used: 10 m³
                 Rate: PHP 15.00/m³
                 TOTAL DUE: PHP 150.00 (Large text)
        
STEP 8: USER presses 'D' to print
        Call printBill()
        Thermal printer outputs:
          - Logo bitmap
          - Company info
          - Account details
          - Meter readings
          - Charges
          - Total amount
          - Footer message
        Reset workflow
        State → STATE_WELCOME
        Display: Welcome screen again
        
System ready for next customer
```

---

## 14. File Dependencies & Includes

### Header File Hierarchy

```
Watersystem_ESP32.ino (Main)
├── config.h
│   └─ Pin definitions, constants
│
├── customers_database.h
│   ├─ Customer struct
│   ├─ customers[] array
│   └─ Lookup functions
│
├── water_system.h
│   ├─ BillData struct
│   ├─ currentBill variable
│   └─ Calculation functions
│
├── workflow_manager.h
│   ├─ WorkflowState enum
│   ├─ State variables
│   ├─ Display functions per state
│   └─ Workflow validation logic
│
├── keypad_manager.h
│   ├─ Keypad object
│   ├─ handleKeypadInput()
│   └─ Route to workflow
│
├── sdcard_manager.h
│   ├─ SD card init
│   └─ Database directory setup
│
└── logo.h
    └─ Bitmap array for thermal printer
```

---

## 15. Testing Checkpoints

### Verification Steps

- [ ] Keypad input accumulates in inputBuffer
- [ ] Account lookup finds existing customers
- [ ] Account lookup rejects non-existent accounts
- [ ] Reading validation rejects readings ≤ previous
- [ ] Bill calculates correctly (usage × rate)
- [ ] TFT displays correct state screens
- [ ] Thermal printer outputs formatted bill
- [ ] Serial commands work (P, D, S, L, START)
- [ ] State transitions occur at correct times
- [ ] Error messages display for 2 seconds
- [ ] System resets to welcome after print
- [ ] Cancel (C) button returns to welcome
