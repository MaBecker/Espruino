How to flash Espruino ESP32P4
=============================

To flash an ESP32 P4 using the serial port use:

```bash
esptool.py -p (PORT) -b 921600 --before default_reset --after hard_reset  \
    --chip esp32s3  write_flash \
    --flash_mode dio --flash_size detect --flash_freq 80m  \
    0x0 bootloader.bin  \
    0x8000 partition-table.bin  \
    0x10000 espruino-esp32s3.bin
```

To flash co board ESP32C6

firmware https://esphome.github.io/esp-hosted-firmware/

```bash
esptool.py -p (PORT) --chip esp32c6 write_flash 0x0 network_adapter_esp32c6.factory.bin
```
