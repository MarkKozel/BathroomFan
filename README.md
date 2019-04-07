# BathroomFan

## Parts
* Adruino Nano
* 5V to 12 V buck converter
* SSD1306 32 x 129 monochrome display
* MQ-5 Gas Sensor

# Behavior
System has a single pushbutton to add 1 minute to the *fan run timer*
Gas readings are run through a filter to normalize sporatic readings

Gas readings will cause 1 minute to be added to *fan run timer* with it exceeds a hard-coded value
