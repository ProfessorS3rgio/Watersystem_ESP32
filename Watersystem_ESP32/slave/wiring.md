## Thermal Printer (UART on ESP32-C3 Mini)

| Printer Pin | ESP32    | Notes                    |
|-------------|----------|--------------------------|
| TX          | GPIO 7   | ESP32 RX <- Printer TX (solid blue) |
| RX          | GPIO 6   | ESP32 TX -> Printer RX (solid green) |
| GND         | GND      | Common ground (solid brown) |
| VCC         | 5V-9V    | Printer power supply (solid orange) |

## Printer Power Control (IRLZ44N MOSFET)

| MOSFET Pin | ESP32    | Notes                                              |
|------------|----------|----------------------------------------------------|
| Gate       | GPIO 1   | Drive gate to switch the 5 V printer supply (safe GPIO on ESP32-C3 Mini) |
| Drain      | Printer VCC | Connect to the printer’s power input through MOSFET |
| Source     | GND      | Common ground                                      |

## Charger State Detection (NPN transistor)

| Node        | ESP32    | Notes                                                |
|-------------|----------|------------------------------------------------------|
| Transistor Collector (output) | GPIO 3   | NPN transistor used to sense charger presence; transistor pulls the pin low when charger active (inverted logic) |

*Note: GPIO 3 is independent from the fuel-gauge I2C bus so no conflict exists.*

## Fuel Gauge (MAX17043 - I2C)

| Gauge Pin | ESP32    | Notes                                                   |
|-----------|----------|---------------------------------------------------------|
| VCC       | 3.3V     | Power                                                   |
| GND       | GND      | Ground                                                  |
| SDA       | GPIO 4   | I2C Data                                                |
| SCL       | GPIO 5   | I2C Clock                                               |
| ALERT     | GPIO 10  | Optional alert output (preferred over strapping pins)   |
| QST       | NC       | Not connected (unused)                                  |
