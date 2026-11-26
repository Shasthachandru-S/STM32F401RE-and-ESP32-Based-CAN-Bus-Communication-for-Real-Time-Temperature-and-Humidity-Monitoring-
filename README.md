# STM32 + ESP32 CAN Bus Communication Using MCP2515
### DHT11 Temperature & Humidity Monitoring System

## Project Overview

This project demonstrates **CAN Bus communication between an STM32F401RE microcontroller and an ESP32 using MCP2515 CAN Controller modules**.

The STM32F401RE reads real-time temperature and humidity data from a DHT11 sensor using register-level (Bare-Metal) programming. The collected sensor values are transmitted through the CAN Bus using the MCP2515 CAN controller.

The ESP32 receives the transmitted CAN data through another MCP2515 module and displays the temperature and humidity readings on a 16x2 I2C LCD. The received information is also displayed on the ESP32 Serial Monitor.

This project demonstrates practical implementation of the CAN communication protocol, SPI interfacing, I2C communication, GPIO configuration, and embedded register-level programming.

## System Architecture

```text
        STM32F401RE
             |
             |
          DHT11
       Temperature &
          Humidity
             |
             |
        SPI Communication
             |
          MCP2515
        CAN Transmitter
             |
             |
        CAN BUS (2 Wires)
          CAN_H / CAN_L
             |
             |
          MCP2515
         CAN Receiver
             |
             |
        SPI Communication
             |
           ESP32
             |
       +-----+------+
       |            |
   Serial Monitor  I2C LCD
                  16x2
```

## Hardware Requirements

| Component | Quantity |
|---|---:|
| STM32F401RE Nucleo Board | 1 |
| ESP32 DevKit V1 | 1 |
| MCP2515 CAN Module | 2 |
| DHT11 Temperature & Humidity Sensor | 1 |
| 16x2 I2C LCD Display | 1 |
| CAN Bus Twisted Pair Wires | 1 Set |
| 120Ω Termination Resistors | 2 |
| Jumper Wires | As required |

## Hardware Connections

### 1. STM32F401RE – DHT11 Sensor

| DHT11 Pin | STM32F401RE |
|---|---|
| VCC | 3.3V |
| DATA | PA1 |
| GND | GND |

### 2. STM32F401RE – MCP2515 CAN Transmitter

| MCP2515 Pin | STM32F401RE Pin |
|---|---|
| VCC | Module-compatible supply |
| GND | GND |
| SCK | PA5 |
| SO (MISO) | PA6 |
| SI (MOSI) | PA7 |
| CS | PB6 |
| INT | Not used |

**Communication Interface:** SPI1

### 3. ESP32 – MCP2515 CAN Receiver

| MCP2515 Pin | ESP32 Pin |
|---|---|
| VCC | Module-compatible supply |
| GND | GND |
| SCK | GPIO 18 |
| SO (MISO) | GPIO 19 |
| SI (MOSI) | GPIO 23 |
| CS | GPIO 5 |
| INT | GPIO 27 |

**Communication Interface:** SPI

### 4. ESP32 – 16x2 I2C LCD

| LCD Pin | ESP32 Pin |
|---|---|
| VCC | 5V (if required by module) |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

**I2C Address:** 0x27

### 5. CAN Bus Connections

| STM32 Side MCP2515 | ESP32 Side MCP2515 |
|---|---|
| CAN_H | CAN_H |
| CAN_L | CAN_L |
| GND | GND |

Use 120Ω termination resistors at both physical ends of the CAN bus.

**Important:** MCP2515 is a CAN controller, not a CAN transceiver. Ensure both modules have compatible CAN transceivers and supply voltage levels. Verify the module's onboard termination and oscillator frequency before powering the system.

## CAN Communication Configuration

| Parameter | Configuration |
|---|---|
| CAN Controller | MCP2515 |
| CAN Protocol | CAN 2.0 |
| CAN Speed | 125 kbps |
| MCP2515 Crystal | 8 MHz |
| CAN Identifier | 0x123 |
| Frame Format | Standard 11-bit ID |
| Data Length | 2 Bytes |

### CAN Data Format

The STM32 transmits two bytes of sensor information.

| Data Byte | Description | Example |
|---|---|---|
| Byte 0 | Temperature | 28°C |
| Byte 1 | Humidity | 65% |

Example CAN Frame:

```text
CAN ID : 0x123
DLC    : 2
DATA   : 28 65
```

The ESP32 extracts the two bytes and displays the corresponding sensor values.

## Software Requirements

### STM32 Development

- STM32CubeIDE
- STM32F401RE Board Support
- CMSIS Device Headers
- STM32F4 Reference Manual
- Bare-Metal Register-Level Programming

### ESP32 Development

- Arduino IDE
- ESP32 Board Package
- SPI Library
- Wire Library
- LiquidCrystal_I2C Library
- MCP_CAN Library

## Program Description

### STM32 Transmitter

The STM32F401RE performs the following operations:

1. Initializes the DWT cycle counter for microsecond delays.
2. Configures GPIO PA1 for DHT11 sensor communication.
3. Initializes I2C1 for LCD communication.
4. Initializes SPI1 for MCP2515 communication.
5. Configures MCP2515 in CAN Normal Mode.
6. Reads temperature and humidity from DHT11.
7. Validates the DHT11 checksum.
8. Packs temperature and humidity into two CAN data bytes.
9. Transmits the CAN frame using Standard CAN ID 0x123.
10. Updates the LCD with sensor readings.

### ESP32 Receiver

The ESP32 performs the following operations:

1. Initializes SPI communication with MCP2515.
2. Initializes the I2C LCD display.
3. Configures MCP2515 at 125 kbps.
4. Sets MCP2515 to CAN Normal Mode.
5. Checks for available CAN messages.
6. Reads incoming CAN frames.
7. Filters messages using CAN ID 0x123.
8. Extracts temperature and humidity values.
9. Displays received values on the Serial Monitor.
10. Displays temperature and humidity on the 16x2 LCD.

## Expected Output

### STM32 LCD Output

```text
Temp: 28 C
Humidity: 65%
```

### ESP32 Serial Monitor Output

```text
CAN receiver ready.
Waiting for STM32 DHT11 data...

CAN ID: 0x123 | Temp: 28 C | Humidity: 65 %
CAN ID: 0x123 | Temp: 29 C | Humidity: 64 %
CAN ID: 0x123 | Temp: 28 C | Humidity: 66 %
```

### ESP32 I2C LCD Output

```text
Temp: 28 C
Humidity: 65%
```

*The displayed readings are illustrative examples.*

## Key Learning Outcomes

- STM32F4 Register-Level Programming
- Bare-Metal Embedded C Programming
- DHT11 Sensor Interfacing
- SPI Protocol Implementation
- I2C Protocol Implementation
- MCP2515 CAN Controller Configuration
- CAN Bus Data Transmission and Reception
- Standard CAN Frame Structure
- ESP32 Hardware Interfacing
- Real-Time Sensor Data Monitoring
- Embedded Systems Communication

## Applications

- Industrial Sensor Monitoring
- Automotive Embedded Systems
- Distributed Sensor Networks
- Industrial Automation
- CAN-Based Data Acquisition Systems
- Microcontroller-to-Microcontroller Communication

## Future Improvements

- Add multiple CAN nodes.
- Implement CAN message filtering.
- Add CAN error detection and recovery.
- Transmit additional sensor parameters.
- Implement CAN interrupt-based reception.
- Add real-time data logging.
- Develop an IoT-based remote monitoring dashboard.

## Author

**Shastha Chandru S**

Embedded Systems Engineer

## License

This project is available for educational and learning purposes. You are welcome to study, modify, and extend the implementation.
