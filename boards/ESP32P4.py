#!/usr/bin/env python3
# This file is part of Espruino, a JavaScript interpreter for Microcontrollers
#
# Copyright (C) 2026 Espruino contributors
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Waveshare ESP32-P4-NANO - initial ESP-IDF 5 board definition.
#
# IMPORTANT:
# - This is a first board-definition draft, not yet a verified build target.
# - The ESP32-P4-NANO uses an ESP32-P4 + ESP32-C6 co-processor.
# - The C6 is connected over SDIO; this does NOT automatically provide
#   Espruino's existing ESP32 WiFi/Bluetooth implementation on the P4.
# - Check the matching IDF5 target makefile and P4-specific jshardware support.

import pinutils

info = {
  'name'                     : "ESP32P4",
  'espruino_page_link'        : 'ESP32',
  'default_console'           : "EV_SERIAL1",
  'default_console_baudrate'  : "115200",
  'variables'                 : 4095,
  'io_buffer_size'            : 4096,
  'binary_name'               : 'espruino_%v_esp32p4.bin',
  'build' : {
    'optimizeflags' : '-Og',
    'libraries' : [
      'ESP32',
      'NET',
      'GRAPHICS',
      'CRYPTO','SHA256', 'SHA512',
      'TLS',
      'TELNET',
      'NEOPIXEL',
      'FILESYSTEM',
      # 'BLUETOOTH'
    ],
    'makefile' : [
      'DEFINES+=-DESP_PLATFORM -DESP32=1 -DESP32P4=1',
      'DEFINES+=-DESP_STACK_SIZE=25000',
      'DEFINES+=-DJSVAR_MALLOC',
      'DEFINES+=-DUSE_FONT_6X8',
      # 16 MB physical flash. Keep firmware/Storage partition sizing in sync
      # with the P4-NANO partition table before enabling Storage.
      'ESP32_FLASH_MAX=15728640'
    ]
  }
}

chip = {
  'part'       : "ESP32P4",
  'family'     : "ESP32_IDF5",
  'package'    : "",
  # ESP32-P4 internal HP SRAM is 768 KB L2 plus additional tightly coupled /
  # low-power memories. Do not advertise PSRAM as ordinary RAM until the
  # P4 IDF heap-capability allocation path is implemented and tested.
  'ram'        : 768,
  'flash'      : 16384,
  'speed'      : 360,
  'usart'      : 3,
  'spi'        : 2,
  'i2c'        : 2,
  'adc'        : 2,
  'dac'        : 0,
}

# No controllable user LED is declared here: the board documentation labels
# the onboard LED as a power indicator. BOOT and RESET are not user GPIOs.
devices = {}

# Header GPIOs from Waveshare's ESP32-P4-NANO pinout.
# Board-wired pins are deliberately omitted from the exposed board diagram:
#   GPIO7/8       onboard shared I2C (codec/touch/camera control)
#   GPIO24..27    USB1 OTG signals (do not use as GPIO while USB is enabled)
#   GPIO37/38     UART0 console
# GPIO0/1 are also marked as 32.768 kHz RTC clock pins on the pinout.
# C6_IO12/13/9 are ESP32-C6 pins, not P4 GPIOs, and are not listed here.
board_p4_nano = {
  'left' : [
    '3V3', '5V', '5V', 'GND',
    'D7', 'D8', 'D23', 'D37', 'D38', 'D4', 'D5',
    'D20', 'D21', 'D22', '3V3', 'D24', 'D25', 'D26',
    'D27', 'D32', 'D33', 'D36'
  ],
  'right' : [
    '5V', 'GND', '3V3', 'GND',
    'D0', 'D1', 'D3', 'D2', 'D6', 'D54', 'D53',
    'D47', 'D48', 'D46', 'D45', 'GND'
  ]
}
boards = [board_p4_nano]

def get_pins():
  # P4 has GPIO0..GPIO54. Keep the generated list complete so pin
  # references and peripheral routing can be added as the P4 backend matures.
  pins = pinutils.generate_pins(0, 54)

  # ADC channels explicitly shown in the Waveshare pinout.
  pinutils.findpin(pins, "PD20", True)["functions"]["ADC1_IN4"] = 0
  pinutils.findpin(pins, "PD21", True)["functions"]["ADC1_IN5"] = 0
  pinutils.findpin(pins, "PD22", True)["functions"]["ADC1_IN6"] = 0
  pinutils.findpin(pins, "PD23", True)["functions"]["ADC1_IN7"] = 0
  pinutils.findpin(pins, "PD53", True)["functions"]["ADC2_IN6"] = 0
  pinutils.findpin(pins, "PD54", True)["functions"]["ADC2_IN7"] = 0

  # UART0 console on the P4-NANO USB-to-UART bridge.
  pinutils.findpin(pins, "PD37", True)["functions"]["USART1_TX"] = 0
  pinutils.findpin(pins, "PD38", True)["functions"]["USART1_RX"] = 0

  # Shared onboard I2C bus (codec/touch/camera control).
  pinutils.findpin(pins, "PD7", True)["functions"]["I2C1_SDA"] = 0
  pinutils.findpin(pins, "PD8", True)["functions"]["I2C1_SCL"] = 0

  # All pins are 3.3 V only; none are 5 V tolerant.
  return pins