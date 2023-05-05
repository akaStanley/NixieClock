// February 1st 2020
// Spencer Kulbacki
// Nixie tube clock with GPS

//bn-180 pinout
//red   VCC (3.0-5.0v)
//green RX (input to chip)
//white TX (output from chip)
//black GND

//https://github.com/mikalhart/TinyGPSPlus/releases
//https://github.com/JChristensen/Timezone

// Include Libraries
#include "SoftwareSerial.h"
#include "TinyGPS++.h"
#include "Timezone.h"

static const int RXPin = 2, TXPin = 3;
//white to pin 3
//green to pin 2
// The serial connection to the GPS device
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

TimeChangeRule *tcr; // pointer to the time change rule, use to get TZ abbrev

bool ClockSetup = true; //used for running initalization loop the first time plugged in
bool TimeCycle = true;

int Tzone = 0; //used for time offset

const int clkPin = 11;   // OC2A output pin is 11 do not change
const int dataMin = 5;
const int dataHr = 6;

const int latchMin = 9;
const int latchHr = 10;

uint8_t prevMin = 00;
uint8_t currentMin = 00;
uint8_t currentHr = 00;

uint8_t ActiveSeconds = 0;

long hourData = 0b00000000000000000000000000000000;
long minuteData = 0b00000000000000000000000000000000;

void setup()
{
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(4, OUTPUT);

  pinMode(clkPin, OUTPUT);
  pinMode(dataHr, OUTPUT);
  pinMode(dataMin, OUTPUT);
  pinMode(latchMin, OUTPUT);
  pinMode(latchHr, OUTPUT);

  digitalWrite(7, HIGH); //pull HIGH minute blank pin so it never blank
  digitalWrite(8, HIGH); //pull HIGH hour blank pin so it never blank
  digitalWrite(13, HIGH); //pull HIGH minute invert pin so it never invert
  digitalWrite(12, HIGH); //pull HIGH hour invert pin so it never invert

  digitalWrite(4, LOW); //pull LOW HV Shutdown so HV is always enabled

  Serial.begin(115200);
  ss.begin(9600);
}

void loop()
{
  //  Serial.println(F("\n\nValue Packet:"));
  //  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
  //  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);

  //East coast is ~-75, so -75/15= -5 hours
  //rough calculation using every 15deg longitude = one hr time zone diff
  //Tzone = ((gps.location.lng()) / 15);
  Tzone = -8;

  TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, ((Tzone + 1) * 60)}; // Daylight time = UTC - 4 hours
  TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, (Tzone * 60)};   // Standard time = UTC - 5 hours
  Timezone myTZ(myDST, mySTD);

  setTimeGPS(gps.date, gps.time);

  time_t local = myTZ.toLocal(now(), &tcr);

  currentMin = getMinute(local);
  currentHr = getHour(local);

  //Debugging 1 of 2
    Serial.print("hour");
    Serial.print(currentHr);
    Serial.print("minute");
    Serial.println(currentMin);

  //time defulats to 4:00 when no GPS data recieved & clock setup has run
  // enter if clock has shown same time for longer than 60 seconds (means loss of GPS)
  if ((currentHr == 4) && (currentMin == 00) && (ClockSetup) || ActiveSeconds >= 61) {
    for (int i = 0; i <= 99; i += 11) {

      timeToBitMinute(i);
      timeToBitHour(i);
      shiftOutData(dataMin, clkPin, latchMin, minuteData);
      shiftOutData(dataHr, clkPin, latchHr, hourData);
      delay(200);
    }
    for (int i = 99; i >= 0; i -= 11) {

      timeToBitMinute(i);
      timeToBitHour(i);
      shiftOutData(dataMin, clkPin, latchMin, minuteData);
      shiftOutData(dataHr, clkPin, latchHr, hourData);
      delay(200);
    }
    ClockSetup = false; //disable clock setup flag for rest of program
  }

  if ((currentHr == 12) && (currentMin == 00) && (TimeCycle)) {
    for (int i = 0; i <= 99; i += 11) {

      timeToBitMinute(i);
      timeToBitHour(i);
      shiftOutData(dataMin, clkPin, latchMin, minuteData);
      shiftOutData(dataHr, clkPin, latchHr, hourData);
      delay(200);
    }
    for (int i = 99; i >= 0; i -= 11) {

      timeToBitMinute(i);
      timeToBitHour(i);
      shiftOutData(dataMin, clkPin, latchMin, minuteData);
      shiftOutData(dataHr, clkPin, latchHr, hourData);
      delay(200);
    }
    TimeCycle = false; //disable clock setup flag for rest of program
  }

  if ((currentHr == 12) && (currentMin == 01)) {
    TimeCycle = true; //reenable for next 12 hour crosspoint to cycle tubes
  }

  //if minute has changed (once every 60 seconds)
  if (currentMin != prevMin) {
    prevMin = currentMin;
    ActiveSeconds = 0;
    timeToBitMinute(currentMin);
    timeToBitHour(currentHr);
    shiftOutData(dataMin, clkPin, latchMin, minuteData);
    shiftOutData(dataHr, clkPin, latchHr, hourData);
  }

  ActiveSeconds += 1; //count up seconds since last minute change (should be no more than 60)
  // Debugging 2 of 2
    Serial.print(ActiveSeconds);

  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}//end main loop

//===================Functions ===============================

void shiftOutData(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, long val)
{
  uint32_t mask = 1L << 31;

  digitalWrite(latchPin, HIGH);

  for (uint8_t i = 32; i > 0; i--)
  {
    digitalWrite(dataPin,  (val & mask) ? HIGH : LOW);    // writes to data pin in MSB order
    //    Serial.print(!!(val & mask));
    mask >>= 1;
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
  }
  digitalWrite(latchPin, LOW);

}//end shiftOutData function

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}//end smart delay function

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');
  }
  smartDelay(0);
}//end printfloat function

static void setTimeGPS(TinyGPSDate & d, TinyGPSTime & t)
{
  //set the time (hr,min,sec,day,mnth,yr)
  setTime(t.hour(), t.minute(), t.second(), d.day(), d.month(), d.year());
}//end setimegps function

// format and print a time_t value, with a time zone appended.
int getHour(time_t t)
{
  if (hour(t) > 12) {
    return (hour(t) - 12);
  }
  else if (hour(t) == 0) {
    return (12);
  }
  else {
    return (hour(t));
  }
}

int getMinute(time_t t)
{
  if (minute(t) < 10 ) {
    return ("%02d", minute(t));
  }
  else {
    return (minute(t));
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
      bitSet(minuteData, 21);
      bitSet(minuteData, 20);
      //  minuteData = 0b00000000000000000000000000000000;
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
      bitSet(minuteData, 21); //dot symbol
      bitSet(minuteData, 20); //dot symbol
      //  minuteData = 0b00000000000000000000000000000000;
      break;
  }

}//end timeToBitMinute function

void timeToBitHour(uint8_t hours)
{
  uint8_t Tens = (hours / 10) % 10;
  uint8_t Ones = (hours % 10);

  //  Use this to get rid of leading zero on Hours (but there will be flicking issues)
  //  if (Tens <= 0)
  //    Tens = 10;
  //  else
  //    Tens = Tens;

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
      hourData = 0b00000000000000000000000000000000;
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
      //bitSet(hourData, 10);
      //  hourData = 0b00000000000000000000000000000000;
      break;
  }

}//end timeToBitHour function
