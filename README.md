# BathroomFan

## Parts
* Adruino Nano
* 5V to 12 V buck converter
* SSD1306 32 x 129 monochrome display
* MQ-5 Gas Sensor

## Links
[Button Wiring](https://www.arduino.cc/en/tutorial/button)
[DHT 11 Temp/RH Sensor](https://create.arduino.cc/projecthub/Arca_Ege/using-dht11-b0f365)
[DHT-Sensor-Library](https://github.com/adafruit/DHT-sensor-library)
[Chrono Time/Stopwatch library](https://github.com/SofaPirate/Chrono)
[Add library .zip file to Adruino](https://www.arduino.cc/en/guide/libraries)

# Behavior
System has a single pushbutton to add 1 minute to the *fan run timer*
Gas readings are run through a filter to normalize sporatic readings

Gas readings will cause 1 minute to be added to *fan run timer* with it exceeds a hard-coded value


