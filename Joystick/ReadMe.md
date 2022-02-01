# K-023 JOYSTICK 


### Components

* ESP-IDF
* ESP32
* Joystick

### DESCRIPTION

* Simple library for the K-023 Joystick tested on esp-idf-v4.0+ with esp32 wrover kit.
* Library shows the direction (North, South, East, West,...) of the Joystick.

**Basics. How Does It Work?**
The analog joystick has a handle that is attached to the joint with two regulators that determine the joysticks location on the X and Y-axes and the button on the Z-axis. The handle’s angle rotates the regulators and changes the output voltage letting it monitor the handle’s gradient from the central spot. When the joystick is released, it smoothly returns to its central (0) location. The value on every analog channel can vary from 0 to 1023. When connected to analog ports, VRx to A0 and VRy to A1 respectively, they should transmit the values as shown in the image below.

<img src="/home/horsemann/Desktop/WorkSpace/esp_idf_4.2/esp-idf/examples/sensor_codes_with_esp32/Joystick/Doc/Basics.jpg" alt="Joystick Working" />

The KY-023 joystick model has its own deficiencies. That’s because the joystick handle doesn’t always return exactly to the central location, so you have to take this into account when programming the handle’s central location as a small range of values, not a specific one. Meaning when the joystick is in the center, the X and Y coordinate values might be in a range from 490 to 530 and not exactly 512.

----------------------------------------------------------------------------------------
**NOTE:**


* When stick is in center, voltage should be half of the vRef. Here we using the ADC_WIDTH_BIT_10, so value range is 0-1023, ideal midpoint value is 512. But practtically, midpoint values in changing between 455-465. Therefore, here the midpoint value is 460.

* Here, default axis range in +X,Y = +512, -X,Y = -512.You can also set the range using the function `set_axis_full_range(512, -512);`


* If you change ADC_WIDTH_BIT_10 to something else, calculate the raw value at midpoint and set the idea value using the function `void set_adc_read_at_centre(int value)`. Value should not be 0 or NULL.

**Described the corresponding pin names.**

- VCC (5v) – Connect this to your positive supply (usually 5V or 3.3V depending on your logic levels).
- VERT (vrY) – This is the vertical analog output voltage (will be about half of VCC when the joystick is centered). **(Default adc channel 6 Pin no. 34)** 
- HORIZ (vrX) – This is the horizontal analog output voltage (will be about half of VCC when the joystick is centered).  **(Default adc channel 7, Pin no. 35)**
- SW (KEY or SEL) – This is the digital output from the pushbutton, normally open, will connect to GND when the button is pushed. **(GPIO Pin 25).**
- GND – Connect this to your ground line (GND).

**NOTE:**
* You can override the default channel and pin values using the function `set_channel_and_button(ADC1_CHANNEL_6, ADC1_CHANNEL_7, 25)` 




