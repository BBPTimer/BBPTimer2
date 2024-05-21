# BBPTimer2
An automatic shot timer for the Breville Bambino Plus*

<sub>\* Should work with any Breville machine with a solenoid valve, but I have only tested on a 2024 Bambino Plus</sub>
# Demo Video
[![BBPTimer Demo](https://i.imgur.com/8vPZHzk.jpeg)](https://youtube.com/shorts/sfyzebhx3to)
# Hardware Requirements
* You will need:
	* A [NodeMCU V2](https://www.amazon.com/HiLetgo-Internet-Development-Wireless-Micropython/dp/B010O1G1ES) board.
	* An [SSD1306](https://www.amazon.com/HiLetgo-Serial-Display-SSD1306-Arduino/dp/B06XRBTBTB) display. Make sure to buy the 4-pin/I2C version.
	* A normally open Reed switch. I use [this switch](https://www.microcenter.com/product/614938/nte-electronics-switch-white-magnetic-alarm-reed-spst) from Micro Center, but also successfully tested with [these switches](https://www.amazon.com/dp/B0B3D7BM4K) from Amazon.
	* [Dupont wires](https://www.amazon.com/Elegoo-EL-CP-004-Multicolored-Breadboard-arduino/dp/B01EV70C78).
	* A 3D-printed [case](https://www.thingiverse.com/thing:2937731). If you don't have a 3D printer, you can use a service like [Craftcloud](https://craftcloud3d.com) to print the case.
# Upload Software
* Connect the NodeMCU board to a computer and use the [Arduino IDE](https://www.arduino.cc/en/software) to upload **BBPTimer2.ino** to the NodeMCU.
	* Most of the included libraries come pre-installed with Arduino IDE. However, you may need to [manually install](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/) a few of the libraries. I commented links to the libraries in the INO file.
# Hardware Setup
* Affix the NodeMCU board and SSD1306 to the internal piece of the case.
* [Connect the SSD1306 display to the NodeMCU board.](https://randomnerdtutorials.com/esp8266-0-96-inch-oled-display-with-arduino-ide/)
* Remove the top lid from the Bambino Plus. To do this, remove the 2 screws at the top rear of the machine, behind the water tank. Pop open the rear of the machine to the topmost 2 tabs, and then slide the top of the machine backwards.
* [Stick the Reed switch to the solenoid valve](https://imgur.com/LNzAFQv).
* Run the Reed switch wires out of the machine. You may need to create a little notch so that the wires have a little "breathing room" on their way out of the machine.
* Attach one of the Reed switch wires to the D7 pin on your NodeMCU, and the other Reed switch wire to a ground pin. It does not matter which Reed switch wire goes to which of those pins.
	* I stripped the ends off of [2 female Dupont wires](#hardware-requirements) and used wire nuts to attach them to the Reed switch wires. This allowed me to attach the Reed switch to the GPIO pins without needing to solder or crimp. [Here is a diagram](https://imgur.com/bvkUthV) (sorry for the terrible photo editing).
* Seat the hardware in the case, and plug the NodeMCU board into power using a Micro USB cable and phone charging brick.
	* I highly recommend using a Command strip or something similar to affix the case to the espresso machine. This will help reduce the risk of accidentally unplugging the Reed switch when you move the espresso machine.
# Software Setup
* Connect to the **BBPTimer** WiFi network using the password **3\$Pr3\$\$0**.
* A window will automatically appear. Click **Configure WiFi**. Enter your WiFi credentials and click **Save**.
	* If the WiFi manager window does not automatically appear, visit **192.168.4.1** in a browser.
* The timer will display **Access settings at \<IP address\>**. Open your browser and visit **\<IP address\>**.
	* The timer will always display its IP address upon boot. If you forget the IP address, just power cycle the timer and it will display again.
* Follow the **Calibration Instructions**.
* If desired, set a PreInfuse time. The display will invert at the end of the pre-infusion time, giving a visual cue to stop pre-infusion.
# Thank You
* I used [marax_timer](https://github.com/alexrus/marax_timer) by Alex Rus as the starting point for this project.
* I borrowed from [this tutorial](https://randomnerdtutorials.com/esp32-esp8266-input-data-html-form/) by Rui Santos for saving form data to SPIFFS.