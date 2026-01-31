/*
This is a simple example to get the heading in degrees (on the horizontal plane) from an IST8310 compass sensor over I2C.
The base code is based on https://github.com/dayaftereh/rocket
Heading calculations are based on https://github.com/sleemanj/HMC5883L_Simple

This example has been tested on Teensy 4.0 board connected to the Holybro F9P Helical module
*/

#include "IST8310.h"

IST8310 ist8310;

void setup() {

  Serial.begin(57600); 
  while (!Serial);   

  Wire.begin();

  Serial.println("Trying to setup IST8310");

  bool success = ist8310.setup(&Wire, &Serial);
  if (!success) {
      Serial.println("Failed to setup IST8310");
  }
  ist8310.set_flip_x_y(false);

  // -0.1887865056 at College Park, MD, USA. This value gets added to the magnetic heading to report the true heading;
  // See http://www.magnetic-declination.com/
  ist8310.set_declination_offset_radians(0.0);  
}

void loop() {

  bool success = ist8310.update();
  if (!success)
  {
      Serial.println("Failed to update IST8310");
  } else {
      Serial.printf("Heading: %f degrees\n", ist8310.get_heading_degrees());  // 0 degrees at true north (or magnetic north if declination is 0)
  }
}