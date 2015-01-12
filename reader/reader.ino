// Basic arduino implementation of the wiegand signaling protocol
// Copyright (C) 2013 Peter Chinetti
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

char data[35];
boolean valid = false;
const int ZERO_PIN = 0;
const int ONE_PIN = 1;
unsigned long timestamp = 0;
int curr_bit = 0;
void setup(){
  attachInterrupt(ZERO_PIN,zero_bit,FALLING);
  attachInterrupt(ONE_PIN,one_bit,FALLING);
  Serial.begin(9600);
}
void loop(){
  if(valid == true){
    noInterrupts();
    int i;
    //Serial.print("Card Number: ");
    for(i=0;i<35;i++){
      Serial.print(data[i]);
    }
    Serial.println();
    curr_bit = 0;
    valid = false;
    interrupts();
  }
}
void zero_bit(){
  if((valid == true) || (millis() - timestamp > 100)){
    valid = false;
    curr_bit = 0;
  }
  data[curr_bit++] = '0';
  if(curr_bit>34){
    valid = true;
  }
  timestamp = millis();
}
void one_bit(){
if((valid == true) || (millis() - timestamp > 100)){
    valid = false;
    curr_bit = 0;
  }
  data[curr_bit++] = '1';
  if(curr_bit>34){
    valid = true;
  }
  timestamp = millis();
}
