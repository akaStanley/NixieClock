// May 3, 2025
// Spencer Kulbacki
// Nixie tube clock with GPS

//bn-180 pinout
//red   VCC (3.0-5.0v)
//green RX (input to chip)
//white TX (output from chip)
//black GND

//Exta Libraries Used:
//https://github.com/mikalhart/TinyGPSPlus
//https://github.com/JChristensen/Timezone
// Versions at compilation & last edit
// Arduino IDE ver 1.8.16
// TimeZone ver 1.2.4
// TinyGPSPlus ver 1.0.3

// Include Libraries
#include "SoftwareSerial.h"
#include "TinyGPSPlus.h"
#include "Timezone.h"
#include "TimeLib.h"  // Arduino library to work with time/date functions

static const int RXPin = 2, TXPin = 3;
//white to pin 3
//green to pin 2
// The serial connection to the GPS device
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

bool TubeCleaning = true;

int TzoneOffset = -8; //used for time offset
time_t prevDisplay = 0; //when the time was last set
int stdOffsetMinutes = 0; //Standard Time offset
int dstOffsetMinutes = 0; //Daylight Time offset

const int clkPin = 11;   // OC2A output pin is 11 do not change
const int dataMin = 5;
const int dataHr = 6;

const int latchMin = 9;
const int latchHr = 10;

uint8_t prevMin = -1;
uint8_t currentMin = 0; //Shown time minute stored here
uint8_t currentHr = 0; //shown time hour stored here

long hourData = 0b00000000000010000000001000000000; //default to show 11
long minuteData = 0b10000000000010000000000000000000; //default to show 11

void setup()
{
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH); //pull HIGH minute blank pin so it never blank
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH); //pull HIGH hour blank pin so it never blank

  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH); //pull HIGH hour invert pin so it never invert
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH); //pull HIGH minute invert pin so it never invert
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW ); //pull LOW HV Shutdown so HV is always enabled

  pinMode(clkPin, OUTPUT);
  digitalWrite(clkPin, LOW);
  pinMode(dataHr, OUTPUT);
  digitalWrite(dataHr, LOW);
  pinMode(dataMin, OUTPUT);
  digitalWrite(dataMin, LOW);
  pinMode(latchMin, OUTPUT);
  digitalWrite(latchMin, LOW);
  pinMode(latchHr, OUTPUT);
  digitalWrite(latchHr, LOW);

  shiftOutData(dataMin, clkPin, latchMin, minuteData); //Send 0 to clear registers
  shiftOutData(dataHr, clkPin, latchHr, hourData); //Send 0 to clear registers

  delay(500); //wait 1/2 second for power to stabalize
  randomSeed(analogRead(A0)); //Read from floating A0 pin to seed Random number gen

  Serial.begin(115200);
  ss.begin(9600);
  Serial.println("Setup Complete");
}

void loop()
{
  while (ss.available()) {
    gps.encode(ss.read());
  }
  static unsigned long lastGPSTask = 0;
  const unsigned long gpsInterval = 500;
  if (millis() - lastGPSTask >= gpsInterval) {
    lastGPSTask = millis();
    if (gps.time.age() >= 10 && gps.date.isValid() && gps.time.isValid() && gps.date.year() > 2000) {

      setTimeGPS(gps.date, gps.time); //sets values when GPS recieved

      if (gps.location.isValid()) {
        //debug print GPS coords
        //Serial.println(F("\n\nGPS Data Packet:"));
        //printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
        //printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);

        float longitude = gps.location.lng(); //Save for math to calculate time zone
        //Eg. East coast is -75, so -75/15= -5 hours, West Coast is -120, so -120/15 = -8 hours
        TzoneOffset = round(longitude / 15.0); //rough calculation using every 15deg longitude = one hr time zone diff
      }
      stdOffsetMinutes = TzoneOffset * 60; //calculate minutes offset based on position
      dstOffsetMinutes = stdOffsetMinutes + 60; // add one 60 minutes for DaylightvSavings offset

      TimeChangeRule myDST = {"Daylight", Second, Sun, Mar, 2, dstOffsetMinutes}; // Daylight time = UTC - offset + 60 min
      TimeChangeRule mySTD = {"Standard", First, Sun, Nov, 2, stdOffsetMinutes};  // Standard time = UTC - offset
      Timezone myTZ(myDST, mySTD); //use daylight savings when matching dates above
      time_t localTime = myTZ.toLocal(now()); //set LocalTime to latest GPS data with Daylight savings & location applied

      currentMin = minute(localTime); //minutes are always minutes
      currentHr = getHour(localTime); //convert to 12hr time (from 24 hr)

      if (currentMin != prevMin) { //if the minute time has changed
        prevMin = currentMin;
        timeToBitMinute(currentMin); //convert time from integer to binary
        timeToBitHour(currentHr);
        shiftOutData(dataMin, clkPin, latchMin, minuteData); //send to tubes via shift register
        shiftOutData(dataHr, clkPin, latchHr, hourData);

        //Debugging:
        //Serial.print("\nTimeDisplayed");
        //Serial.print("\nhour ");
        //Serial.println(currentHr);
        //Serial.print("minute ");
        //Serial.println(currentMin);
        //Serial.print("sec ");
        //Serial.println(second());
        //End debugging
      }

    }
    else {
      //Serial.println(F("Waiting for GPS fix..."));
      DisplayRandomPattern(10); //show 10 random numbers on display
    }
  }


  //tube cleanging sequence, runs every 12 hours
  if (currentHr == 12 && currentMin == 02 && TubeCleaning == true) {
    for (int i = 0; i <= 99; i += 11) { // Increment up by 11
      //Serial.print("Tube Cleaning: ");
      //Serial.println(i);
      timeToBitMinute(i);
      timeToBitHour(i);
      shiftOutData(dataMin, clkPin, latchMin, minuteData);
      shiftOutData(dataHr, clkPin, latchHr, hourData);
      delay(500);
    }
    for (int i = 88; i >= 0; i -= 11) { // Decrement down by 11
      //Serial.print("Tube Cleaning: ");
      //Serial.println(i);
      timeToBitMinute(i);
      timeToBitHour(i);
      shiftOutData(dataMin, clkPin, latchMin, minuteData);
      shiftOutData(dataHr, clkPin, latchHr, hourData);
      delay(500);
    }
    TubeCleaning = false; // End the tube cleaning after loop completes
  }

  if (currentHr == 12 && currentMin == 10) {
    TubeCleaning = true; //setup for next crossover point
  }
}//end main loop

//===================Functions ===============================
//Send data to shift registers
void shiftOutData(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, uint32_t val)
{
  uint32_t mask = 1UL << 31;

  // Begin shifting data
  digitalWrite(latchPin, LOW);  // Keep latch low while loading data

  for (uint8_t i = 0; i < 32; i++) {
    // Write data bit (MSB first)
    digitalWrite(dataPin, (val & mask) ? HIGH : LOW);
    delayMicroseconds(1); //settle
    digitalWrite(clockPin, HIGH);    // Clock pulse
    delayMicroseconds(1); //settle
    digitalWrite(clockPin, LOW);
    delayMicroseconds(1); //settle
    mask >>= 1;
  }

  // Latch the data to outputs
  digitalWrite(latchPin, HIGH);
  delayMicroseconds(1); //settle
  digitalWrite(latchPin, LOW);
}
//PrintFloat Used for GPS Lat/Long Debugging
//static void printFloat(float val, bool valid, int len, int prec)
//{
//  if (!valid)
//  {
//    while (len-- > 1)
//      Serial.print('*');
//    Serial.print(' ');
//  }
//  else
//  {
//    Serial.print(val, prec);
//    int vi = abs((int)val);
//    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
//    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
//    for (int i = flen; i < len; ++i)
//      Serial.print(' ');
//  }
//}//end printfloat function


static void DisplayRandomPattern(int numCycles) {
  int randMin = 0;
  int randHr = 0;

  for (int i = 0; i < numCycles; i += 1) {
    randMin = random(0, 99);
    randHr = random(0, 99);
    timeToBitMinute(randMin); //show random number on minutes
    timeToBitHour(randHr); //show random number on hour
    shiftOutData(dataMin, clkPin, latchMin, minuteData);
    shiftOutData(dataHr, clkPin, latchHr, hourData);
    delay(100);
  }
}//end random pattern display function


static void setTimeGPS(TinyGPSDate & d, TinyGPSTime & t)
{
  //set the time (hr,min,sec,day,mnth,yr)
  setTime(t.hour(), t.minute(), t.second(), d.day(), d.month(), d.year());
}//end setimegps function

int getHour(time_t localTime)
{
  int hourIn = hour(localTime);
  if (hourIn > 12) {
    return (hourIn - 12);
  }
  else if (hourIn == 0) {
    return (12);
  }
  else {
    return (hourIn);
  }
}

void timeToBitMinute(uint8_t minutes)
{

  uint8_t Tens = (minutes / 10) % 10;
  uint8_t Ones = (minutes % 10);

  //Bit values are 0-31 in program, address pins on HV5622 1-32

  minuteData = 0b00000000000000000000000000000000;
  switch (Ones) {
    case 0:
      bitSet(minuteData, 10);
      break;
    case 1:
      bitSet(minuteData, 19);
      break;
    case 2:
      bitSet(minuteData, 18);
      break;
    case 3:
      bitSet(minuteData, 17);
      break;
    case 4:
      bitSet(minuteData, 16);
      break;
    case 5:
      bitSet(minuteData, 15);
      break;
    case 6:
      bitSet(minuteData, 14);
      break;
    case 7:
      bitSet(minuteData, 13);
      break;
    case 8:
      bitSet(minuteData, 12);
      break;
    case 9:
      bitSet(minuteData, 11);
      break;
    default:
      bitSet(minuteData, 11); //9
      // minuteData = 0b00000000000000000000000000000000;
      break;
  }
  switch (Tens) {
    case 0:
      bitSet(minuteData, 22);
      break;
    case 1:
      bitSet(minuteData, 31);
      break;
    case 2:
      bitSet(minuteData, 30);
      break;
    case 3:
      bitSet(minuteData, 29);
      break;
    case 4:
      bitSet(minuteData, 28);
      break;
    case 5:
      bitSet(minuteData, 27);
      break;
    case 6:
      bitSet(minuteData, 26);
      break;
    case 7:
      bitSet(minuteData, 25);
      break;
    case 8:
      bitSet(minuteData, 24);
      break;
    case 9:
      bitSet(minuteData, 23);
      break;
    default:
      bitSet(minuteData, 24); //8
      //minuteData = 0b00000000000000000000000000000000;
      break;
  }

}//end timeToBitMinute function

void timeToBitHour(uint8_t hours)
{
  uint8_t Tens = (hours / 10) % 10;
  uint8_t Ones = (hours % 10);

  //Bit values are 0-31 in program, address pins on HV5622 1-32

  hourData = 0b00000000000000000000000000000000;
  switch (Ones) {
    case 0:
      bitSet(hourData, 0);
      break;
    case 1:
      bitSet(hourData, 9);
      break;
    case 2:
      bitSet(hourData, 8);
      break;
    case 3:
      bitSet(hourData, 7);
      break;
    case 4:
      bitSet(hourData, 6);
      break;
    case 5:
      bitSet(hourData, 5);
      break;
    case 6:
      bitSet(hourData, 4);
      break;
    case 7:
      bitSet(hourData, 3);
      break;
    case 8:
      bitSet(hourData, 2);
      break;
    case 9:
      bitSet(hourData, 1);
      break;
    default:
      bitSet(hourData, 2); // 8
      //hourData = 0b00000000000000000000000000000000;
      break;
  }
  switch (Tens) {
    case 0:
      bitSet(hourData, 10);
      break;
    case 1:
      bitSet(hourData, 19);
      break;
    case 2:
      bitSet(hourData, 18);
      break;
    case 3:
      bitSet(hourData, 17);
      break;
    case 4:
      bitSet(hourData, 16);
      break;
    case 5:
      bitSet(hourData, 15);
      break;
    case 6:
      bitSet(hourData, 14);
      break;
    case 7:
      bitSet(hourData, 13);
      break;
    case 8:
      bitSet(hourData, 12);
      break;
    case 9:
      bitSet(hourData, 11);
      break;
    default:
      bitSet(hourData, 11); //9
      //  hourData = 0b00000000000000000000000000000000;
      break;
  }

}//end timeToBitHour function
