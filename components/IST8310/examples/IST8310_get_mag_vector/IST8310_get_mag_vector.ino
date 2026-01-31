/*
This is a simple example to get magnetometer values from an IST8310 compass sensor over I2C.
The code is almost entirely based on https://github.com/dayaftereh/rocket

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
  ist8310.set_flip_x_y(true);
}

void loop() {

  bool success = ist8310.update();
  if (!success)
  {
      Serial.println("Failed to update IST8310");
  } else {
      Vec3f* mag_latest_val = ist8310.get_magnetometer();

      Serial.printf("Mag (x, y, z): (%f, %f, %f)\n", mag_latest_val->x, mag_latest_val->y, mag_latest_val->z);
  }
}