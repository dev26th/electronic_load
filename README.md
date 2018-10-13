# Electronic Load
Reinvented firmware for the electronic load 60W. With PC control Software (Windows/Linux).

Status: **beta**.

The load can be ordered from China (AliExpress, eBay etc), something like this one:

![The electronic load](docs/1.jpg)

## Added features
* full control via UART (connected to pins T and R)
* calibration (via direct EEPROM programming)
* parameters (e.g. minimal current) are changable
* bootloader can be enabled

## Programming
Programming the chip in the device has some drawbacks:
* you cannot return to original firmware if you don't like this one
* you cannot use a simple USB-UART adapter to program it first time

Also the datasheet claims a write endurance of only 100 flash cycles but this is probably only for marketing purposes as it [contains the same die](https://hackaday.io/project/16097-eforth-for-cheap-stm8s-gadgets/log/76731-stm8l001j3-a-new-sop8-chip-and-the-limits-of-stm8flash) as the STM8S105 which is rated for 10000 cycles.

### Programmer connection
Connect an STLink V2 to the pins at the bottom of the board:

![Programmer connection](docs/2.jpg)

### Flashing
If you are using the chip which is already on the board you first have to unlock it via 

    make unlock
 
Then you write the new firmware with 

    make flash
 
and if you are flashing for the first time you also have to program the EEPROM:

    make eeprom
 

[Analog part schematic](http://www.voltlog.com/pub/dummy-load-sch.pdf) (corrections: PB3 is connected to +12V via 20k, R27 is 510 Ohm).
[Full schematic](https://github.com/ArduinoHannover/ZPB30A1_Firmware/raw/master/hardware/schematic.pdf) (one correction: polarity of D6 is reversed).

Calibration values in the code are for my instance, may be you have to adjust them for your one.

