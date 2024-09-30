#include "LedControl.h"
#include "DHT.h"
#include "RTClib.h"

RTC_DS1307 rtc;

#define MAX_LED 4
/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(12, 10, 11, MAX_LED);

const uint8_t NUMBER[10][4] = {
  {0b00111110, 0b01000001, 0b01000001, 0b00111110}, // 0
  {0b00010001, 0b00100001, 0b01111111, 0b00000001}, // 1
  {0b00110011, 0b01000101, 0b01001001, 0b00110001}, // 2
  {0b00100010, 0b01001001, 0b01001001, 0b00110110}, // 3
  {0b01110000, 0b00001000, 0b00001000, 0b01111111}, // 4
  {0b01110010, 0b01010001, 0b01010001, 0b01001110}, // 5
  {0b00111110, 0b01001001, 0b01001001, 0b00100110}, // 6
  {0b01000000, 0b01000000, 0b01000000, 0b01111111}, // 7
  {0b00110110, 0b01001001, 0b01001001, 0b00110110}, // 8
  {0b00110010, 0b01001001, 0b01001001, 0b00111110}, // 9
};
const uint8_t NUMBER_2 [10][3] = {
  {0b00011111, 0b00010001, 0b00011111}, // 0
  {0b00001001, 0b00011111, 0b00000001}, // 1
  {0b00010111, 0b00010101, 0b00011101}, // 2
  {0b00010101, 0b00010101, 0b00011111}, // 3
  {0b00011100, 0b00000100, 0b00011111}, // 4
  {0b00011101, 0b00010101, 0b00010111}, // 5
  {0b00011111, 0b00010101, 0b00010111}, // 6
  {0b00010000, 0b00010111, 0b00011000}, // 7
  {0b00011111, 0b00010101, 0b00011111}, // 8
  {0b00011101, 0b00010101, 0b00011111}, // 9
};
const uint8_t CHARACTER[3] = {0b00000000, 0b00010100, 0b00000000}; // " : "
const uint8_t CHARACTER_2[4] = {0b00011000, 0b00011000, 0b00000111, 0b00000101}; // *C
const uint8_t SPACE[4] = {0b00000000, 0b00000000, 0b00000000, 0b00000000};

int8_t hour = 0, minus = 0, second = 0;
uint16_t year, month, day;
uint8_t timeArr[6];

uint32_t timerEdit;
boolean isHideNumber = false, isEditClock = false, isEditMode = false;
uint8_t indexEdit = 0;
uint8_t modeDisplay = 2;

const int DHTPIN = 2;       //Đọc dữ liệu từ DHT11 ở chân 2 trên mạch Arduino
const int DHTTYPE = DHT11;  //Khai báo loại cảm biến, có 2 loại là DHT11 và DHT22
 
DHT dht(DHTPIN, DHTTYPE);

uint8_t temperature = 0, countTemperature = 0;
float temperatureAvr = 0;
uint32_t timerReadTemperature;

boolean buttonFunc, buttonFuncLast = true, buttonUp, buttonDown;
uint32_t timerFunc, timerButtonUp, timerButtonDown;


void setup() {
  Serial.begin(9600);
  Serial.println("Begin");
  for (int i=0; i<MAX_LED; i++) {
    /*
    The MAX72XX is in power-saving mode on startup,
    we have to do a wakeup call
    */
    lc.shutdown(i,false);
    /* Set the brightness to a medium values */
    lc.setIntensity(i,4);
    /* and clear the display */
    lc.clearDisplay(0);
  }
  dht.begin();
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  for (int i=3; i<=5; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  timerReadTemperature = millis();
  timerEdit = millis();
}

void clearDisplay() {
  for (int i=0; i<4; i++) {
    for (int j=0; j<8; j++) {
      lc.setColumn(i, j, 0b00000000);
    }
  }
}

void changeMode() {
  buttonFunc = digitalRead(3);
  if (!buttonFunc && buttonFuncLast) {
    buttonFuncLast = buttonFunc;
    timerFunc = millis();
  } else if (buttonFunc && !buttonFuncLast) {
    if (!isEditMode) {
      if (!isEditClock) {
        clearDisplay();
        modeDisplay = modeDisplay == 1 ? 2 : 1;
      } else {
        indexEdit += 2;
        if (modeDisplay == 1) {
          if (indexEdit > 2) {
            indexEdit = 0;
          }
        } else {
          if (indexEdit > 4) {
            indexEdit = 0;
          }
        }
      }
    } else {
      isEditMode = false;
    }
    buttonFuncLast = buttonFunc;
  }
  if (!buttonFunc && (millis() - timerFunc > 3000) && !isEditMode) {
    isEditClock = !isEditClock;
    if (!isEditClock) {
      rtc.adjust(DateTime(year, month, day, hour, minus, second));
    }
    isEditMode = true;
    indexEdit = 0;
    timerFunc = millis();
  }
}

void editClock() {
  buttonUp = digitalRead(4);
  buttonDown = digitalRead(5);
  if (!buttonUp && (millis() - timerButtonUp > 300)) {
    switch (indexEdit)
    {
    case 0:
      hour++;
      if (hour>23) {
        hour = 0;
      }
      break;
    case 2:
      minus++;
      if (minus>59) {
        minus = 0;
      }
      break;
    case 4:
      second++;
      if (second>59) {
        second = 0;
      }
      break;
    default:
      break;
    }
    timerButtonUp = millis();
  }
  if (!buttonDown && (millis() - timerButtonDown > 300)) {
    switch (indexEdit)
    {
    case 0:
      hour--;
      if (hour<0) {
        hour = 23;
      }
      break;
    case 2:
      minus--;
      if (minus<0) {
        minus = 59;
      }
      break;
    case 4:
      second--;
      if (second<0) {
        second = 59;
      }
      break;
    default:
      break;
    }
    timerButtonDown = millis();
  }
}

void readDataClock() {
  DateTime now = rtc.now();
  year = now.year();
  month = now.month();
  day = now.day();
  hour = now.hour();
  minus = now.minute();
  second = now.second();
}

void readDataDHT() {
  if (millis() - timerReadTemperature > 1500) {
    temperatureAvr += dht.readTemperature();
    countTemperature++;
    if (countTemperature >= 2) {
      temperature = temperatureAvr/countTemperature;
      if (temperature > 99) {
        temperature = 99;
      }
      countTemperature = 0;
      temperatureAvr = 0;
    }
    timerReadTemperature = millis();
  }
}

void splitData() {
  if (hour < 10) {
    timeArr[0] = 0;
    timeArr[1] = hour;
  } else {
    timeArr[0] = hour / 10;
    timeArr[1] = hour % 10;
  }
  if (minus < 10) {
    timeArr[2] = 0;
    timeArr[3] = minus;
  } else {
    timeArr[2] = minus / 10;
    timeArr[3] = minus % 10;
  }
  if (modeDisplay == 1) {
    if (temperature < 10) {
      timeArr[4] = 0;
      timeArr[5] = temperature;
    } else {
      timeArr[4] = temperature / 10;
      timeArr[5] = temperature % 10;
    }
  } else {
    if (second < 10) {
      timeArr[4] = 0;
      timeArr[5] = second;
    } else {
      timeArr[4] = second / 10;
      timeArr[5] = second % 10;
    }
  }
}

void displayMatrix() {
  uint8_t maxCharDisplay = modeDisplay == 1 ? 7 : 8;
  uint8_t indexMatrix = 3, startColumn = 0, endColumn = 4;
  uint8_t count = 0;
  uint8_t number = 0;
  uint8_t numChar = 4;
  boolean isDisplaySpace = false;
  boolean isDisplayTemperature = false;
  uint8_t countFinalDisplay = 0;
  uint8_t countChar = 0;
  uint8_t numDisplayFinal = 0;
  while(countChar < maxCharDisplay) {
    if (countFinalDisplay == 0) {
      if (countChar == 2 || (countChar == 5 && modeDisplay == 2)) {
        numChar = 3;
        isDisplaySpace = true;
        isDisplayTemperature = false;
      } else if ((countChar == 5 || countChar == 6) && modeDisplay == 1) {
        numChar = 3;
        isDisplaySpace = false;
        isDisplayTemperature = true;
      } else  {
        numChar = 4;
        isDisplaySpace = false;
        isDisplayTemperature = false;
      }
    } else {
      numChar = countFinalDisplay;
    }
    if (countChar == 5 && modeDisplay == 1) {
      count+=2;
    }
    startColumn = count;
    endColumn = count + numChar > 8 ? 8 : startColumn + numChar;
    if (count >= 8) {
      startColumn = 0;
      endColumn = numChar;
      count = 0;
      indexMatrix--;
      if (indexMatrix < 0) {
        indexMatrix = 0;
      }
    }
    for (int i=startColumn; i<endColumn; i++) {
      if (isDisplaySpace) {
        lc.setColumn(indexMatrix, i, CHARACTER[i-startColumn]);
      } else if (isDisplayTemperature) {
        lc.setColumn(indexMatrix, i, NUMBER_2[timeArr[number]][i-startColumn + numDisplayFinal]);
      } else {
        if (isHideNumber && isEditClock && (number == indexEdit || number == indexEdit + 1)) {
          lc.setColumn(indexMatrix, i, SPACE[i-startColumn]);
        } else {
          lc.setColumn(indexMatrix, i, NUMBER[timeArr[number]][i-startColumn + numDisplayFinal]);
        }
      }
      count++;
    }
    countFinalDisplay = numChar - (endColumn - startColumn);
    numDisplayFinal = countFinalDisplay != 0 ? 8 - startColumn : 0;
    if (!isDisplaySpace && countFinalDisplay == 0) {
      number++;
    }
    if (countFinalDisplay == 0) {
      countChar++;
    }
  }
  if (modeDisplay == 1) {
    // Display character *C
    for (int i=4; i<=7; i++) {
      lc.setColumn(0, i, CHARACTER_2[i-4]);
    }
  }
}

void loop() {
  changeMode();
  if (isEditClock) {
    editClock();
  } else {
    readDataClock();
  }
  readDataDHT();
  splitData();
  displayMatrix();
  
  if (millis() - timerEdit > 250 && isEditClock) {
    isHideNumber = !isHideNumber;
    timerEdit = millis();
  }
}