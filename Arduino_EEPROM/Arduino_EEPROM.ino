// AT28C64 EEPROM Programmer by Carsten Herting 07.08.2020
// IMPORTANT: For a stable write process, use a power supply >= 5V for the EEPROM (NOT Arduino 5V!)

#define EEPROM_BYTESIZE   0x2000
#define SET_OE(state)     bitWrite(PORTB, 0, state)   // must be high for write process
#define SET_WE(state)     bitWrite(PORTB, 1, state)   // must be a 100-1000ns low pulse
#define READ_DATA         (((PIND & 0b01111100) << 1) | (PINC & 0b00000111))

void setup()
{
  PORTC = 0; DDRC = 0;
  PORTD = 0; DDRD = 0;
  DDRB = 0b00111111;
  Serial.begin(115200);
}

void loop()
{
  PORTB = 0b00100011;           // LED on, /WE=HIGH, /OE=HIGH
  if (Serial.available() != 0)
  {
    switch(Serial.read())
    {
      case 'w':
        Serial.write('W'); bitWrite(PORTB, 5, 0);       // LED off
        for(int adr=0; adr<EEPROM_BYTESIZE; adr+=32)
        {
          while (Serial.available() < 32);
          for(int i=0; i<32; i++)
          {
            byte dat = Serial.read();                   // read byte
            while (WriteEEPROM(adr|i, dat) == false);   // try writing it
          }
          Serial.write(1);                              // send handshake
        }          
        break;
      case 'r':        
        Serial.write('R'); bitWrite(PORTB, 5, 0);       // LED off
        ToRead();
        SET_OE(LOW);                                    // activate EEPROM outputs
        for(int i=0; i<EEPROM_BYTESIZE; i++) { SetAddress(i); Serial.write(READ_DATA); }
        SET_OE(HIGH);                                   // deactivate EEPROM outputs
        break;
      default: break;
    }
  }      
}
    
void SetAddress(int adr)
{ 
  for (byte i=0; i<16; i++)
  {
    bitWrite(PORTB, 2, adr & 1); adr = adr>>1;
    bitWrite(PORTB, 3, LOW); bitWrite(PORTB, 3, HIGH);
  }
  bitWrite(PORTB, 4, HIGH); bitWrite(PORTB, 4, LOW);
}

void ToRead()
{
  DDRC = 0b00000000;            // set to input and switch off pull-ups
  PORTC = 0b00000000;
  DDRD &= 0b10000011;
  PORTD &= 0b10000011;
}

void WriteTo(byte data)
{
  DDRC = 0b00000111;          // set to outputs
  PORTC = data & 0b00000111;
  DDRD |= 0b01111100;
  PORTD = (PIND & 0b10000011) | ((data & 0b11111000) >> 1);
}

bool WriteEEPROM(int adr, byte data)
{
  noInterrupts();
  SetAddress(adr);
  WriteTo(data);
  SET_OE(HIGH);             // deactivate EEPROM outputs
  SET_WE(HIGH);             // was HIGH before
  SET_WE(LOW);              // 625ns LOW pulse (spec: 100ns - 1000ns)
  SET_WE(LOW);
  SET_WE(LOW);
  SET_WE(LOW);
  SET_WE(LOW);
  SET_WE(HIGH);             // rising edge: data latch, write process begins  
  interrupts();
  ToRead();  
  SET_OE(LOW);              // activate the output for data polling
  int c = 0; while (READ_DATA != data && c < 30000) c++;    // warten (meist Erfolg bei < 5000)
  SET_OE(HIGH);             // deactivate the outputs
  if (c < 30000) return true; else return false;
}

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Carsten Herting
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
