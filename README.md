# STM32 Ethernet Cable Tester with LCD Display

A comprehensive solution for testing Ethernet cables, identifying cable types (Straight/Crossover), and detecting wiring faults using an STM32 microcontroller and I2C LCD display.

## Features

- **Cable Testing**:
  - Detect Straight-through cables
  - Identify T568B Crossover cables
  - Find faulty connections (broken/short-circuited wires)
  - Display results on 16x2 LCD
- **Additional Functionality**:
  - RGB LED color cycling control
  - Morse code messaging via UART
  - Mode switching between testing and RGB control

## Hardware Requirements

- STM32F103C8T6 (Blue Pill) development board
- 16x2 I2C LCD Display (PCF8574 backpack)
- 2x RJ45 connectors with breakout boards
- 3x LEDs (RGB) with current-limiting resistors
- Tactile button
- Ethernet cables for testing
- ST-Link V2 programmer

## Pin Connections

| STM32 Pin | Component       | Connection          |
|-----------|-----------------|---------------------|
| PA1-PA8   | RJ45 Side A     | Pins 1-8            |
| PB8-PB15  | RJ45 Side B     | Pins 1-8            |
| PC13      | Test Button     | Button input        |
| PB6       | I2C1 SCL        | LCD SCL             |
| PB7       | I2C1 SDA        | LCD SDA             |
| PB0       | Red LED         | RGB Red channel     |
| PB1       | Green LED       | RGB Green channel   |
| PA8       | Blue LED        | RGB Blue channel    |

## Setup Instructions

1. **Clone Repository**:
   ```bash
   git clone https://github.com/yourusername/stm32-cable-tester.git

    Open in STM32CubeIDE:

        Import project into STM32CubeIDE

        Ensure STM32F1xx HAL libraries are installed

    Build & Flash:

        Connect ST-Link programmer

        Build project and flash to microcontroller

    Hardware Setup:
    plaintext

    RJ45 Side A -> PA1-PA8 (Pins 1-8)
    RJ45 Side B -> PB8-PB15 (Pins 1-8)
    LCD I2C Address: 0x3F (configured in liquidcrystal_i2c.h)

Usage

    Cable Testing:

        Connect cable between RJ45 connectors

        Press button:

        Short press: Run test

        Long press (>1s): Toggle between test/RGB modes

    LCD Display:

        Good cable: Type: Straight or Type: Crossover

        Faulty cable: FAULTY CABLE with bad wire numbers

    RGB Control:

        In RGB mode, short press cycles through colors:

            Mode A: Green gradients

            Mode B: Red gradients

            Mode C: Blue gradients

    Serial Commands (115200 baud):

        T: Toggle test/RGB modes

        A/B/C: Switch RGB modes

        morse: Trigger Morse code demo

Project Structure
plaintext

stm32-cable-tester/
├── Core/
│   ├── Inc/           # Header files
│   └── Src/           # Main source files
├── Drivers/           # HAL libraries
├── USB_DEVICE/        # USB CDC configuration
└── STM32F103C8TX_FLASH.ld # Linker script

Troubleshooting

    LCD Not Working:

        Verify I2C address (try 0x27/0x3F)

        Check SDA/SCL connections

    False Cable Readings:

        Ensure 220Ω resistors in test lines

        Verify RJ45 pin connections

    Button Unresponsive:

        Check PC13 pull-up configuration
