# Nixie Tube Clock
From scratch design of a Nixie Tube Clock, with a twist. This clock sets its time using a GPS reciever, so it has the correct time anywhere around the world--and you don't have set it.

Read the "Notes on Nixie Tube Clock.rtf" for more details on the actual design. Here are some images of the design.
All the Code, PCB, and 3D model files are included here if you want to make your own. BOM cost will run you around 300$ (as of May 2025)

### The Finnished Clock
![Clock Final](/Images/Clock_final.jpg)

### The Clock Powered Off
![Clock 3/4 view powered off](/Images/Clock-Front.JPG)

### PCB Top side
![Board Top side](/Images/BoardTop.PNG)

### PCB Bottom side (with fix)
![Board Bottom side with added 5v LDO](/Images/pcb_bottom.JPG)

### Circuit Overview
![3/4 view of the GPS module, MCU and power supply](/Images/Circuit2.JPG)

### Circuit Detail
From left to right:  
12vDC to 180v boost circuit Max1771, HV shift register 1 for Hours, arduino MCU, GPS antenna, HV shift register 2 for minutes (underneath GPS)  
![Top view of the GPS module, MCU and power supply](/Images/Circuit1.JPG)

## 3D Printing
I bought an Ender 3 V2 and modeled a base and some trim parts for this clock.  

### "Naked" Tube legs
![Ugly](/Images/Legs1.JPG)

### Angled Base
![Perfect viewing angle](/Images/base1.jpg)

### 3D printed ring to cover them
![Elegant](/Images/legs2.jpg)

## Tubes!
Like any old electronics one day they will _stop working_ ... and they aren't getting any cheaper to repair.  
In 2020, the larger IN-18 tubes were 50$ each, and the smaller IN-8-2 were a measly 18$. Shipped from a wearhouse in Belarus.  
Flash forward to 2025, and the large tubes are 85$ and the small IN-8-2 are now retailing for 25$ each.  
![IN-8-2 on the left, IN-18 on the right](/Images/Tubes.jpg)  

![Tube testing, hoping to bring one back from the dead](/Images/TubeTesting.JPG)  
