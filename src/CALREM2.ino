#include <LiquidCrystal.h>
#include<IRremote.h>
#include <DS1302.h>
#include<EEPROM.h>

#define SLOT_SIZE 23
int hh, mi, ss, dd, mm, yyyy, lastPrinted, newInput = 0;
String remText, ampm = String("");

IRrecv irrecv(2);
decode_results signals;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

DS1302 rtc(12, 11, 10);

void setup() {
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("CALREM SYS-Aan");
  irrecv.enableIRIn();
  rtc.halt(false);
  rtc.writeProtect(false);
}

void loop() {
  /*
     1. Update time to screen
     2. Check for CH button
     3. Check if there is any reminder
  */
  //Part 1
  echoTime();
  
  //Part 2
  checkForCH();
  //Part 3
  checkReminder();
  
  delay(1000);
}

void echoTime() {
  Time t;
  //Time structure has year, mon, date, hour, min, sec
  t.date=0;

  //RTC module sometimes returns null values, we must keep requesting until actual value is passed
  while(t.date==0){
    t=rtc.getTime();
  }
  
  //RTC returns 24H time, must show in 12H format for ease of use :)
  if(t.hour>12){ampm = "PM"; t.hour-=12;}
  else if(t.hour==12){ampm="PM";}
  else if(t.hour==0){ampm="AM";t.hour=12;}
  else ampm = "AM";

  //don't refresh time until its more than 5 secs from last echo. Saves unnecessary refreshing/flickering of screen
  if(millis()-lastPrinted < 5000) return;
  lastPrinted=millis();
  lcd.clear();
  lcd.setCursor(4, 0);
  char buffer[16];
  sprintf(buffer, "%02d:%02d", t.hour, t.min);
  lcd.print(String(buffer)+" "+ampm);
  sprintf(buffer, "%02d/%02d/%04d", t.date, t.mon, t.year);
  lcd.setCursor(3, 1);
  lcd.print(buffer);
}


void checkForCH() {
  int pressed = signalDecode();
  if(newInput && pressed==11){
    newInput=0;
    displayMenu();
  }
}

int signalDecode() {
  int sigVal = 0;
  int ms = millis();

  //allot upto 1 second for fetching input
  do{
    if (irrecv.decode(&signals)) {
    newInput = 1;
    break;
    }
    else {
    newInput = 0;
    }
  }while (millis() - ms <= 1000);

  if (newInput == 0) return 0; //return if there was no new input

  sigVal = signals.value;

  switch (sigVal) {
    case 0xFF6897:
      Serial.println("0");
      sigVal = 0;
      break;
    case 0xFF30CF:
      Serial.println("1");
      sigVal = 1;
      break;
    case 0xFF18E7:
      Serial.println("2");
      sigVal = 2;
      break;
    case 0xFF7A85:
      Serial.println("3");
      sigVal = 3;
      break;
    case 0xFF10EF:
      Serial.println("4");
      sigVal = 4;
      break;
    case 0xFF38C7:
      Serial.println("5");
      sigVal = 5;
      break;
    case 0xFF5AA5:
      Serial.println("6");
      sigVal = 6;
      break;
    case 0xFF42BD:
      Serial.println("7");
      sigVal = 7;
      break;
    case 0xFF4AB5:
      Serial.println("8");
      sigVal = 8;
      break;
    case 0xFF52AD:
      Serial.println("9");
      sigVal = 9;
      break;
    case 0xFF906F:
      Serial.println("EQ");
      sigVal = 10;
      break;
    case 0xFF629D:
      Serial.println("CH");
      sigVal = 11;
      break;
    case 0xFFE01F:
      Serial.println("-");
      sigVal = 12;
      break;
    case 0xFFA857:
      Serial.println("+");
      sigVal = 13;
      break;
  }
  if (sigVal < 0 || sigVal > 13) sigVal = 14; //all other inputs alloted 14 as key number
  
  delay(200);
  irrecv.resume();
  return sigVal;
}

void displayMenu() {
  int choice;
  Serial.println("MENU: 1.TIME 2.REM");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1. SET TIME");
  lcd.setCursor(0, 1);
  lcd.print("2. SET REMINDER");
  while (1) {
    newInput=0;
    while (newInput != 1) {
      choice = signalDecode();
    }
    newInput=0;
    if (choice == 1) {
      setRTCTime();
      break;
    }
    else if (choice == 2) {
      setReminder();
      break;
    }
    else if (choice == 11) {
      break;
    }
    else {
      lcd.clear();
      lcd.print("INVALID INPUT");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("1. SET TIME");
      lcd.setCursor(0, 1);
      lcd.print("2. SET REMINDER");
      continue;
    }
  }
}

void inputRTCTime(String setWhat) {
  hh = askInput(String("SET ") + setWhat + String(" 24HH?"), 2);
  mi = askInput(String("SET ") + setWhat + String(" MM?"), 2);
  ss = askInput(String("SET ") + setWhat + String(" SS?"), 2);
  dd = askInput(String("SET ") + setWhat + String(" DD?"), 2);
  mm = askInput(String("SET ") + setWhat + String(" MM?"), 2);
  yyyy = askInput(String("SET ") + setWhat + String(" YYYY?"), 4);
  if (!verifyDateTime()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INVALID DATETIME");
    lcd.setCursor(0,1);
    lcd.print(String(hh)+String(mi)+String(ss)+String(dd)+String(mm)+String(yyyy));
    delay(7000);
    inputRTCTime(setWhat);
    return;
  }
}

void setRTCTime() {
  inputRTCTime("TIME");
  rtc.setTime(hh, mi, ss);
  rtc.setDate(dd, mm, yyyy);
}

void setReminder() {
  /*
    1. Check if reminder(s) already there
    2. If there, ask for overwrite/go back
    3. Enter reminder date/time
    4. Enter reminder text
  */
  int slotChoice = 1;
  while (1) {
    slotChoice = askInput(String("REM SLOT(1/2/3)?"), 1);
    if (slotChoice < 1 || slotChoice > 3) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INVALID SLOT NUM");
      delay(1000);
      continue;
    }
    if (checkSlotFree(slotChoice)) break;
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Slot is full");
      delay(1000);
      if (askInput(String("1.OVWRITE 2.BACK"), 1) == 1) {
        eraseSlot(slotChoice); break;
      }
      else {
        continue;
      }
    }
  }
  inputRTCTime("REM");
  remText = askInputText("REMINDER TEXT?", 16);
  writeToSlot(slotChoice);
}

void readFromSlot(int slot) {
  int slotStart = SLOT_SIZE * (slot - 1);
  remText = String("");
  if (!EEPROM.read(slotStart)) return;
  hh = EEPROM.read(slotStart + 1); mi = EEPROM.read(slotStart + 2); ss = EEPROM.read(slotStart + 3);
  dd = EEPROM.read(slotStart + 4); mm = EEPROM.read(slotStart + 5); yyyy = EEPROM.read(slotStart + 6) + 2000;
  for (int i = 0; i < 16; i++) {
    remText.concat((char)EEPROM.read(slotStart + 7 + i));
  }
}

void writeToSlot(int slot) {
  int slotStart = SLOT_SIZE * (slot - 1);
  EEPROM.write(slotStart, 1);
  EEPROM.write(slotStart + 1, (byte)hh); EEPROM.write(slotStart + 2, (byte)mi); EEPROM.write(slotStart + 3, (byte)ss);
  EEPROM.write(slotStart + 4, (byte)dd); EEPROM.write(slotStart + 5, (byte)mm); EEPROM.write(slotStart + 6, (byte)yyyy-2000);
  for (int i = 0; i < remText.length(); i++) {
    EEPROM.write(slotStart + 7 + i, remText.charAt(i));
  }
}

void eraseSlot(int slot) {
  EEPROM.write(SLOT_SIZE * (slot - 1), 0);
}

int checkSlotFree(int slot) {
  if ( EEPROM.read(SLOT_SIZE * (slot - 1)) == 1 ) return 0;
  return 1;
}

int verifyDateTime() {
  if (hh > 23 || hh < 0 || mi > 59 || mi < 0 || ss > 59 || ss < 0 ) {
    setAndPrint(0,"PROB IN HH/MM/SS");return 0;
  }
  else {
    if (mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) {
      if (dd > 31 || dd < 1)
      {setAndPrint(0,"PROB IN DD/MM 1");return 0;}
    }
    else {
      if (dd > 30 || dd < 1)
      {setAndPrint(0,"PROB IN DD/MM 1");return 0;}
    }
  }
  return 1;
}

int askInput(String prompt, int digits) {
  int tempVal = 0;
  int tempDigit = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(prompt);
  lcd.setCursor(0, 1);

  for (int i = digits - 1; i >= 0; i--) {
    newInput=0;
    while (newInput != 1) {
      tempDigit = signalDecode();
    }
    newInput=0;
    if ((tempDigit < 0 || tempDigit > 9)) {

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INVALID KEYPRESS");
      delay(2000);

      
      return askInput(prompt, digits);
    }

    tempVal += power(10, i) * tempDigit;

    lcd.print(tempDigit);

  }
  lcd.print(" OK");
  delay(500);
  return tempVal;
}

int power(int a, int n) {
  int tempVal = 1;
  for (int i = 0; i < n; i++) {
    tempVal = tempVal * a;
  }
  return tempVal;
}

String askInputText(String prompt, int maxlength) {
  //input args: Prompt to show
  int len = 0, keyPressed;
  char current = 'A';
  String inputString = String("");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(prompt);
  setAndPrint(len, current);

  while (len < maxlength) {
    newInput=0;
    while (newInput != 1) {
      keyPressed  = signalDecode();
    }
    newInput=0;
    if (keyPressed == 10) {
      inputString.concat(current);
      len++;
      current = 'A';
      setAndPrint(len, current);
    }
    else if (keyPressed == 12) {
      current--;
      if (current == 64) current = (char)122;
      setAndPrint(len, current);
    }
    else if (keyPressed == 13) {
      current++;
      if (current == 123) current = (char)65; //Plus button on 'z' (lowercase) loops to 'A' (uppercase)
      setAndPrint(len, current);
    }
    else if (keyPressed == 11) {
      break;
    }
  }
  if (len < maxlength) {
    for (int i = len; i < maxlength; i++) {
      inputString.concat(' ');
    }
  }
  return inputString;
}

void setAndPrint(int len, char current) {
  lcd.setCursor(len, 1);
  lcd.print(current);
}

void checkReminder() {
  for (int i = 1; i <= 3; i++ ) {
    readFromSlot(i);
    int remind = isRemTime();
    if (remind && checkSlotFree(i)==0) reminderDisplay(i);
  }
}

int isRemTime() {
  Time t;
  t.date=0;
  while(t.date==0){t=rtc.getTime();}
  //year, mon, date, hour, min, sec
  long tRTC = dateSecs(t.hour, t.min, t.sec, t.date, t.mon, t.year);
  long tRem = dateSecs(hh, mi, ss, dd, mm, yyyy);
//  Serial.println(tRTC-tRem);
//  lcd.clear();
//  lcd.print(String(tRTC)+String(t.mon)+String(t.date)+String(t.year-2000));
//  lcd.setCursor(0,1);
//  lcd.print(String(tRem)+String(mm)+String(dd)+String(yyyy-2000));
//  delay(2000);
//  lcd.clear();
  if (tRem <= tRTC) {
    return 1;
  }
  return 0;
}

void reminderDisplay(int slot) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("REM REJECT-(EQ)");
  lcd.setCursor(0, 1);
  lcd.print(remText);
  delay(1000);
  int keyPressed;
  while (1) {
    newInput=0;
    while(newInput!=1) keyPressed = signalDecode();
    newInput=0;
    if (keyPressed == 10){
      eraseSlot(slot);
      break;
    }
  }
}

long dateSecs(int h, int i, int s, int d, int m, int y) {
  int monthDayCount[] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
  long tRem = ((y - 2000) * 365 + ((y - 2000 - 1) / 4 + 1) + ((m > 1) ? (monthDayCount[m - 1]) : 0) + (((y % 4 == 0) && (m > 2)) ? 1 : 0) + d) * 86400 + h * 3600 + i * 60 + s;
  return tRem;
}
