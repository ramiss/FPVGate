# NovaLab Boards

NovaLab boards are an easy way to wire up an esp32-c3-supermini and an RX5808. The case fits the m1.2 screw holes and the usb port on the esp32.

## Parts Needed

* 5 M1.2 screws
* NovaLab PCB Board
* esp32-c3 supermini
* RX5808 (or equivilent)
* MSK12C02 switch (https://www.amazon.com/IRLML6402-10pieces-MSK12C02-BB-SMD/dp/B0F29XVV9F)

## Install steps

* Solder the esp32 and switch on the top side, and the RX5808 on the bottom side
* 3D Print the case in PETG if possible for better heat handling
* Upload the latest StarForge OS Firmware using the flash tool and esp32-c3-supermini image

## Debug

* Some versions of the esp32-c3 supermini have a flawed antenna design that means their wifi signal is very poor. (https://done.land/components/microcontroller/families/esp/esp32/developmentboards/esp32-c3/c3supermini/#caveat-defective-board-designs). If you have one of these flawed boards unfortunately software patches cannot fully resolve the issue, only help it be slightly less terrible.