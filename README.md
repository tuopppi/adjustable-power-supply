# Adjustable current limited power supply

![Power supply](https://dl.dropbox.com/u/4476572/photos/power-supply.jpg)

## Specs
Output voltage: 1.25 - 11V   
Max output current: 1.5A   

## Features
4-digit seven segment display, capable of showing   
   * Voltage
   * Current
   * Current limit

Pushable digital adjustment knobs  
Current limiting   

## Design
[![Schematic](https://dl.dropbox.com/u/4476572/photos/power-supply-schema.png)](https://github.com/tuopppi/adjustable-power-supply/blob/master/power-supply-schematic.pdf)  
![PCB](https://dl.dropbox.com/u/4476572/photos/power-supply-pcb.png)

Atmel ATmega328 microcontroller  
Low side current sensing  
Voltage set by PWM generated by uC  

