/*
LAB SES 1 High Altitude Balloon downlink firmware written by VE3SVF.

***NEED TO FINISH*** camera trigger system. It has not been tested yet. 

This is not quite yet tested on real hardware (YET!)

Thanks:
https://wiki-content.arduino.cc/en/Tutorial/BuiltInExamples/BlinkWithoutDelay
https://reference.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

for the non-blocking camera help!!!

*/

#include <string.h>
#include <util/crc16.h>
#include <Servo.h>

char datastring[80];
int cycle_num = 1; //This is used to keep track of what to transmit in the RTTY beacon, telemetry or reception stuff
unsigned long cycles = 0; //Originally an int object, but it gets long *fast*
const long interval = 100; 
unsigned long previousMillis = 0;  
int ledState = LOW;  
const int ledPin = 8; //Camera pin

Servo myservo;

void setup() {
  pinMode(3,OUTPUT); //RTTY output
  pinMode(ledPin, OUTPUT); //Camera trigger
  myservo.attach(9); 
  myservo.write(0);
  Serial.begin(9600);
  Serial.print("Hello. Starting!");
}

void loop() {
  if (cycle_num == 1){
    //Serial.print("Start loop 1");
    sprintf(datastring,"LAB SES 1 CALLING PLEASE SEND REPORTS TO SASHA.NYC09 AT GMAIL.COM");
    rtty_txstring (datastring);
    char datastring[80];
    //Serial.print("Finished loop 1");
    
  }
  if (cycle_num == 2){
    sprintf(datastring,"TLM: ");
    char voltage_string[10]; //Just to be safe!
    int voltage = readVccaverage();
    dtostrf(voltage, 4, 0, voltage_string);
    //Serial.print(voltage);
    //Serial.print(voltage_string);
    strcat(datastring,voltage_string);
    rtty_txstring (datastring); //transmit it
    char datastring[80];
    cycle_num = 0;
    
  }
  if (cycles == 2000){
    //CUTDOWN TIMEEEEEEEE
    //Um... cutdown!
    sprintf(datastring,"CUTDOWN IS A GO");
    rtty_txstring (datastring);
    
    myservo.write(60);
    delay(500);
    myservo.write(0);
    delay(500);
    myservo.write(70);
    delay(500);
    myservo.write(0);
    delay(500);
    myservo.write(80);
    delay(500);
    myservo.write(0);
    delay(500);
    //Above: let's make sure everything works! :) :)
    sprintf(datastring,"DEPLOY PARACHUTE SUCCESS");
    rtty_txstring (datastring);
  }
  cycle_num++;
  cycles++;
  Serial.print(cycles);
  Serial.print("\n");
    
}



void rtty_txstring (char * string)
{
 
  /* Simple function to sent a char at a time to 
     ** rtty_txbyte function. 
    ** NB Each char is one byte (8 Bits)
    Also a camera trigger function. This is because it runs fast enough, but not TOO fast.
    */
 
  char c;
  
  
 
  c = *string++;
 
  while ( c != '\0')
  {
    rtty_txbyte (c);
    c = *string++;
    //cycles++; //Camera stuff
      

  }
}
 
 
void rtty_txbyte (char c)
{
  /* Simple function to sent each bit of a char to 
    ** rtty_txbit function. 
    ** NB The bits are sent Least Significant Bit first
    **
    ** All chars should be preceded with a 0 and 
    ** proceded with a 1. 0 = Start bit; 1 = Stop bit
    **
    */
 
  int i;
 
  rtty_txbit (0); // Start bit
 
  // Send bits for for char LSB first 
 
  for (i=0;i<7;i++) // Change this here 7 or 8 for ASCII-7 / ASCII-8
  {
    if (c & 1) rtty_txbit(1); 
 
    else rtty_txbit(0); 
 
    c = c >> 1;
 
  }
 
  rtty_txbit (1); // Stop bit
  rtty_txbit (1); // Stop bit
   //Trigger camera
  unsigned long currentMillis = millis(); //Camera
  if (currentMillis - previousMillis >= interval) {
     previousMillis = currentMillis;
      if (ledState == LOW) {
          ledState = HIGH;
      } 
      else {
        ledState = LOW;
      }
   digitalWrite(ledPin, ledState);
   Serial.print(ledState);
   Serial.print("\n");
        
    }
}
 
void rtty_txbit (int bit)
{
  if (bit)
  {
    // high
    tone(3,870);
  }
  else
  {
    // low
    tone(3,700);
 
  }
 
  //                  delayMicroseconds(3370); // 300 baud
  delayMicroseconds(10000); // For 50 Baud uncomment this and the line below. 
  delayMicroseconds(10150); // You can't do 20150 it just doesn't work as the
                            // largest value that will produce an accurate delay is 16383
                            // See : http://arduino.cc/en/Reference/DelayMicroseconds
 
}
 
uint16_t gps_CRC16_checksum (char *string)
{
  size_t i;
  uint16_t crc;
  uint8_t c;
 
  crc = 0xFFFF;
 
  // Calculate checksum ignoring the first two $s
  for (i = 2; i < strlen(string); i++)
  {
    c = string[i];
    crc = _crc_xmodem_update (crc, c);
  }
 
  return crc;
}  

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

float average (int * array, int len)  // assuming array is int.
{
  long sum = 0L ;  // sum will be larger than an item, long for safety.
  for (int i = 0 ; i < len ; i++)
    sum += array [i] ;
  return  ((float) sum) / len ;  // average will be fractional, so float may be appropriate.
}

long readVccaverage(){
  int average_array[255]; //Averaging array
  long result;
  for (int i = 0; i <= 255; i++) {
    //Average 255 times
    average_array[i] = readVcc();
  }
  result = average(average_array,255);
  return result;
}
