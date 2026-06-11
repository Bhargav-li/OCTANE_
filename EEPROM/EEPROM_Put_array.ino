/***

    eeprom_put example.

    This shows how to use the EEPROM.put() method.

    Also, this sketch will pre-set the EEPROM data for the

    example sketch eeprom_get.

    Note, unlike the single byte version EEPROM.write(),

    the put method will use update semantics. As in a byte

    will only be written to the EEPROM if the data is actually

    different.

    Written by Christopher Andrews 2015

    Released under MIT licence.

***/

#include <EEPROM.h>

struct MyObject {

  float field1;

  byte field2;

  char name[10];
};
 
void setup() {

  Serial.begin(9600);

  while (!Serial) {

    ; // wait for serial port to connect. Needed for native USB port only

  }

  int array[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  
  /** Put is designed for use with custom structures also. **/

  //Data to store.


  
  int eeAddress = 0;

  EEPROM.put(eeAddress, array);

  Serial.print("Written custom data type! \n\nView the example sketch eeprom_get to see how you can retrieve the values!");
}


void loop() {

  /* Empty loop */
}
