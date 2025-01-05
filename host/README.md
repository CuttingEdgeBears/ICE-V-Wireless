# Command Line Application for Firmware V0.4

This directory contains source for the host-side Linux command-line utility which
can be used to communicate with the ICE-V Wireless board over USB. There is currently
one application available:
* send_c3usb - communicates with the board over a local USB connection and
allows setting WiFi credentials as well as flashing, "instant" loading and
miscellaneous housekeeping functions.

## Building for Linux

The application is built from source using the standard C compiler that is available
for most Linux distribution. Simply type
```
make
```

## Running under Linux

The application `send_c3usb` communicates with the ICE-V-Wireless board via
USB and requires only that you attach the board to a host computer and wait
until the green `C3` LED is flashing. 

### Usage

```
send_c3usb <options> [FILE]
  FILE is a file containing the FPGA bitstream to ICE-V
  -b gets the current battery voltage
  -f write bitstream to SPIFFS instead of to FPGA
  -p <port> specify the CDC port (default = /dev/ttyACM0)
  -r <reg> read FPGA register
  -w <reg> <data> write FPGA register
  -v enables verbose progress messages
  -V prints the tool version
```

### Fast FPGA programming

To quickly load a new configuration into the FPGA

```
.\send_c3usb <bitstream>
```

### Update default power-on configuration

To load a new default configuration into SPIFFS for loading at power-up

```
.\send_c3usb -f <bitstream>
```

### Read battery voltage

To get the current LiPo batter voltage value in millivolts

```
.\send_c3usb -b
```

### Read a SPI register

If the current FPGA design supports SPI CSRs, read a register

```
.\send_c3usb -r <REG>
```

### Write a SPI register

If the current FPGA design supports SPI CSRs, write a register

```
send_c3usb -w <REG> <DATA>
```

