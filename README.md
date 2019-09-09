# GpsWithGsmLocation
Arduino project to send current GPS location using GSM shield

== Step 0: Before start
- In secret.h set Your server for example #define SECRET_SERVER "your-server-addres.com"

== Step 1: Installation

For GPS module:
 - Connect VCC to 3.3V
 - RX from gps module to pin 10
 - TX from gps module to pin 11
 - GND to GND

For GSM module:
 - 5VIN to 5V 
 - GND to GND 
 - TX to pin 2 Arduino (if not working replace pins at c. and d.
 - RX to pin 3 Arduino
