## Thermal Printer (UART2)

| Printer Pin | ESP32    | Notes                    |
|-------------|----------|--------------------------|
| TX          | GPIO 17  | ESP32 RX ← Printer TX (solid blue) |
| RX          | GPIO 16  | ESP32 TX → Printer RX (solid green) |
| DTR         | GPIO 5   | ESP32 DTR → Printer DTR (stripe blue) |
| GND         | GND      | Common ground (solid brown) |
| VCC         | 5V-9V    | Printer power supply (solid orange) |

## RTC Module (DS3231 - I2C)

| RTC Pin | ESP32    | Notes                      |
|---------|----------|----------------------------|
| VCC     | 3.3V     | Power                      |
| GND     | GND      | Ground                     |
| SDA     | GPIO 26  | I2C Data                   |
| SCL     | GPIO 27  | I2C Clock                  |
| NC      | -        | Not Connected             |
