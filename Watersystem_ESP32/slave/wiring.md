## Thermal Printer (UART2)

| Printer Pin | ESP32    | Notes                    |
|-------------|----------|--------------------------|
| TX          | GPIO 17  | ESP32 RX ← Printer TX (solid blue) |
| RX          | GPIO 16  | ESP32 TX → Printer RX (solid green) |
| GND         | GND      | Common ground (solid brown) |
| VCC         | 5V-9V    | Printer power supply (solid orange) |

## Printer Power Control (IRLZ44N MOSFET)

| MOSFET Pin | ESP32    | Notes                                              |
|------------|----------|----------------------------------------------------|
| Gate       | GPIO 14  | Drive gate to switch the 5 V printer supply        |
| Drain      | Printer VCC | Connect to the printer’s power input through MOSFET |
| Source     | GND      | Common ground                                      |

## Charger State Detection (NPN transistor)

| Node        | ESP32    | Notes                                                |
|-------------|----------|------------------------------------------------------|
| Transistor Collector (output) | GPIO 23  | NPN transistor used to sense charger presence; transistor pulls the pin low when charger active (inverted logic) |

*Note: GPIO 23 is unrelated to the fuel‑gauge; ALERT remains on GPIO 25 so no conflict exists.*

## Fuel Gauge (MAX17043 - I2C)

| Gauge Pin | ESP32    | Notes                                                   |
|-----------|----------|---------------------------------------------------------|
| VCC       | 3.3V     | Power                                                   |
| GND       | GND      | Ground                                                  |
| SDA       | GPIO 26  | I2C Data                                                |
| SCL       | GPIO 27  | I2C Clock                                               |
| ALERT     | GPIO 25  | Optional alert output                                   |
| QST       | GPIO 4   | Optional quick‑start input; tie to a free GPIO (e.g. GPIO 4) for manual quick-start pulses, or connect to ground/reset if unused |
