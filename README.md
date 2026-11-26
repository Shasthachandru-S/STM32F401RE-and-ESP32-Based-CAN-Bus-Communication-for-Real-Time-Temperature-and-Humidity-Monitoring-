# Multi-Node CAN Bus Based Environmental Monitoring System

**Author:** SHASTHA CHANDRU S

## Project Overview

This project demonstrates sensor-data communication between two different microcontroller nodes using the CAN (Controller Area Network) protocol.

The **STM32 is the first node (transmitter)**. It reads temperature and humidity from a DHT sensor and sends the measured data through a CAN bus. The **ESP32 is the second node (receiver)**. It receives the CAN message and reads the temperature and humidity values sent by the STM32.

The main objective is to demonstrate reliable data exchange between an STM32 and an ESP32 using CAN communication.

## System Architecture

```text
       NODE 1 – TRANSMITTER
       --------------------
       DHT Temperature &
       Humidity Sensor
               |
               v
          STM32 Board
        (Reads DHT Data)
               |
              SPI
               |
         MCP2515 Module
               |
          CAN Bus
        CAN_H / CAN_L
               |
         MCP2515 Module
               |
              SPI
               |
          ESP32 Board
        (Receives CAN Data)
               |
               v
       NODE 2 – RECEIVER
```

## Node Description

### Node 1 – STM32 Transmitter

- Controller: STM32 development board
- Sensor: DHT temperature and humidity sensor
- CAN interface: MCP2515 CAN controller module
- Reads temperature and humidity values from the DHT sensor.
- Prepares the sensor readings as a CAN data message.
- Transmits the data through the CAN bus using the MCP2515 module.

### Node 2 – ESP32 Receiver

- Controller: ESP32 development board
- CAN interface: MCP2515 CAN controller module
- Receives the CAN data transmitted by the STM32 node.
- Extracts the temperature and humidity values from the received CAN message.
- Displays or outputs the received readings for monitoring.

## Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| STM32 development board | 1 | Reads sensor data and transmits it |
| ESP32 development board | 1 | Receives and processes CAN data |
| DHT sensor | 1 | Measures temperature and humidity |
| MCP2515 CAN module | 2 | CAN communication interface for both nodes |
| LCD display | As used | Displays sensor readings |
| CAN bus wires | As required | Connects the two CAN nodes |
| Breadboard and jumper wires | As required | Hardware prototyping |

*Confirm the exact STM32 board model and DHT sensor model from the hardware markings before adding their part numbers.*

## Communication Workflow

1. The DHT sensor measures the surrounding temperature and humidity.
2. The STM32 reads the sensor values.
3. The STM32 sends the readings to its MCP2515 module through SPI.
4. The MCP2515 transmits the data over the CAN bus.
5. The MCP2515 connected to the ESP32 receives the CAN frame.
6. The ESP32 reads the received data through SPI.
7. The ESP32 extracts the temperature and humidity values for monitoring.

## Communication Interfaces

- **DHT interface:** STM32 reads temperature and humidity from the DHT sensor.
- **SPI:** Communication between each microcontroller and its MCP2515 module.
- **CAN:** Data communication between the STM32 transmitter node and ESP32 receiver node.

## Key Features

- Two-node CAN bus communication.
- STM32-based temperature and humidity data acquisition.
- CAN data transmission from STM32 to ESP32.
- ESP32-based CAN data reception.
- Demonstration of SPI interfacing with MCP2515.
- Distributed embedded-system communication.

## Hardware Connection Notes

The exact GPIO and SPI pin numbers depend on the selected STM32 and ESP32 boards and the actual wiring. Add the verified pin mapping to this section.

- Connect the STM32 to its MCP2515 module using SPI.
- Connect the ESP32 to its MCP2515 module using SPI.
- Connect CAN_H to CAN_H and CAN_L to CAN_L between the two CAN modules.
- Ensure both CAN nodes use the same CAN bitrate and compatible CAN settings.
- Use 120-ohm termination at the two physical ends of the CAN bus, as required by the bus topology.
- Verify supply voltage and logic-level compatibility before powering the modules.

## Software and Development Tools

- STM32 development environment used for the transmitter firmware.
- Arduino IDE or the selected ESP32 development environment for receiver firmware.
- DHT sensor library or driver, if used.
- MCP2515 CAN library/driver compatible with the selected firmware platform.

## Repository Structure

```text
Multi-Node-CAN-Bus-Environmental-Monitoring/
├── README.md
├── STM32_Transmitter/
│   └── STM32 source code
├── ESP32_Receiver/
│   └── ESP32 source code
├── Hardware/
│   └── Circuit and pin connections
└── Images/
    └── Project photographs
```

## Project Objective

To acquire temperature and humidity data using an STM32 microcontroller and transfer the sensor readings to an ESP32 through a CAN bus, demonstrating communication between two embedded controller nodes.

## Author

**SHASTHA CHANDRU S**
