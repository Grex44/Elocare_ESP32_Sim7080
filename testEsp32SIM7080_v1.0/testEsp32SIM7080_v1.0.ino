/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */
#include "FS.h"
#include "SPIFFS.h"
// #include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_NeoPixel.h>
#include <MPU6050_light.h>
#include <I2C_RTC.h>

/*
0x34  = axp202
0x51  = pcf8563 
0x68  = IMU
0x77  = BMe280
*/

#define FORMAT_SPIFFS_IF_FAILED true


Adafruit_BME280 bme;  // I2C
unsigned long delayTime;
const int GSR = 10;
int sensorValue = 0;
int gsr_average = 0;
int gsr_ohm = 0;

// digital pin 12 has a pushbutton attached to it.
const int pushButton = 6;

// digital pin 27 has a LED indicator attached to it.
// const int LEDindicator = 38;

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 38  // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 1  // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500  // Time (in milliseconds) to pause between pixels

const int buzzer = 17;  //buzzer to arduino pin 9


// Variables will change:
int buttonPushCounter = 0;  // counter for the number of button presses
int lastButtonState = 0;    // previous state of the button
uint16_t buttonLog = 0;     //button being logged

//IMU variable
MPU6050 mpu(Wire);

//BME variable
#define SEALEVELPRESSURE_HPA (1013.25)

// //SD card pinout and variables
// #define SCK   12
// #define MISO  13
// #define MOSI  11
// #define CS    41

// SPIClass spi = SPIClass(VSPI);

//RTC
static PCF8563 RTC;

//Memory
int count = 0;
char fName[20];
bool memoryState;

// Variables will change:
int buttonPushCounter = 0;  // counter for the number of button presses
int lastButtonState = 0;    // previous state of the button
uint16_t buttonLog = 0;     //button being logged


void setup() {
  Serial.begin(115200);
  unsigned status1;
  Wire.begin(3, 4);
  pinMode(pushButton, INPUT);
  pinMode(buzzer, OUTPUT);  // Set buzzer - pin 9 as an output
  Serial.println("Start up...");

  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  Serial.println("pixels started..");
  byte status = mpu.begin();
  Serial.println("MPU started..");
  delay(500);
  mpu.calcOffsets(true, true);  // gyro and accelero
  status1 = bme.begin(0x76);

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  if (!status1) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bme.sensorID(), 16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  //RTC initialize
  RTC.begin();
  RTC.setDay(8);
  RTC.setMonth(12);
  RTC.setYear(2022);
  RTC.setWeek(7);  // Always Set weekday after setting Date

  RTC.setHours(0);
  RTC.setMinutes(0);
  RTC.setSeconds(0);

  Serial.println("*** RTC PCF8563 ***");
  Serial.print("Is Clock Running : ");
  if (RTC.isRunning())
    Serial.println("Yes");
  else
    Serial.println("No");

  Serial.print("Alarm Enabled  : ");
  if (RTC.isAlarmEnabled())
    Serial.println("Yes");
  else
    Serial.println("No");

  Serial.print("Alarm Triggered : ");
  if (RTC.isAlarmTriggered())
    Serial.println("Yes");
  else
    Serial.println("No");

  Serial.print("Timer Enabled  : ");
  if (RTC.isTimerEnabled())
    Serial.println("Yes");
  else
    Serial.println("No");

  //setup of SPIFFS
  listDir(SPIFFS, "/", 0);
  //  writeFile(SPIFFS, "/dLog0.txt", "Hello ");

  sprintf(fName, "/dLog0.txt");
  File file;
  if (!SPIFFS.exists(fName)) file = SPIFFS.open("/dLog0.txt", FILE_WRITE);
  else {
    for (int i = 1; i < 200; i++) {
      sprintf(fName, "/dLog%d.txt", i);
      if (!SPIFFS.exists(fName)) {
        file = SPIFFS.open(fName, FILE_WRITE);
        delay(10);
        break;
      }
      //      file.close();
    }
  }
  Serial.println("hello");
  // if (!rtc.begin()) {
  //   Serial.println("Couldn't find RTC");
  //   // Serial.flush();
  //   while (1) delay(10);
  // }

  // if (rtc.lostPower()) {
  //   Serial.println("RTC is NOT initialized, let's set the time!");
  //   // When time needs to be set on a new device, or after a power loss, the
  //   // following line sets the RTC to the date & time this sketch was compiled
  //   // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //   // This line sets the RTC with an explicit date & time, for example to set
  //   // January 21, 2014 at 3am you would call:
  //   // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  //   //
  //   // Note: allow 2 seconds after inserting battery or applying external power
  //   // without battery before calling adjust(). This gives the PCF8523's
  //   // crystal oscillator time to stabilize. If you call adjust() very quickly
  //   // after the RTC is powered, lostPower() may still return true.
  // }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  // rtc.start();
  startBuzz();
  Serial.println("Booting up...");
}

void loop() {
  int buttonState = digitalRead(pushButton);
  long sum = 0;

  File file = SPIFFS.open(fName, FILE_APPEND);

  // DateTime now = rtc.now();
  Serial.println("starting...");
  mpu.update();
  // put your main code here, to run repeatedly:
  pixels.clear();  // Set all pixel colors to 'off'

  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
  // Here we're using a moderately bright green color:
  // pixels.setPixelColor(0, pixels.Color(0, 150, 0));

  pixels.show();  // Send the updated pixel colors to the hardware.

  Serial.println("Lights completed ...");

  //GSR//
  for (int i = 0; i < 10; i++)  //Average the 10 measurements to remove the glitch
  {
    sensorValue = analogRead(GSR);
    sum += sensorValue;
    delay(5);
  }
  gsr_average = sum / 10;
  gsr_ohm = ((1024 + 2 * gsr_average) * 10000) / (512 - gsr_average);

  //Serial output
  Serial.print(RTC.getDay());Serial.print("-");Serial.print(RTC.getMonth());Serial.print("-");Serial.print(RTC.getYear());
  Serial.print(" ");
  Serial.print(RTC.getHours());Serial.print(":");Serial.print(RTC.getMinutes());Serial.print(":");Serial.print(RTC.getSeconds());
  Serial.print("; T:");
  Serial.print(mpu.getTemp());
  Serial.print("; AccX:");
  Serial.print(mpu.getAccX());
  Serial.print("; AccY:");
  Serial.print(mpu.getAccY());
  Serial.print("; AccZ:");
  Serial.print(mpu.getAccZ());
  Serial.print("; GyroX:");
  Serial.print(mpu.getGyroX());
  Serial.print("; GyroY:");
  Serial.print(mpu.getGyroY());
  Serial.print("; GyroZ:");
  Serial.print(mpu.getGyroZ());
  Serial.print("; Pres:");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.print("; Alt:");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.print("; Hum:");
  Serial.print(bme.readHumidity());
  Serial.print("; GsrAvg:");
  Serial.print(gsr_average);
  Serial.print("; ohm:");
  Serial.println(gsr_ohm);
  // Serial.println(F("=====================================================\n"));

  //file print//
  file.print(RTC.getDay());file.print("-");file.print(RTC.getMonth());file.print("-");file.print(RTC.getYear());
  file.print(" ");
  file.print(RTC.getHours());file.print(":");file.print(RTC.getMinutes());file.print(":");file.print(RTC.getSeconds());
  file.print("; T:");
  file.print(mpu.getTemp());
  file.print("; AccX:");
  file.print(mpu.getAccX());
  file.print("; AccY:");
  file.print(mpu.getAccY());
  file.print("; AccZ:");
  file.print(mpu.getAccZ());
  file.print("; GyroX:");
  file.print(mpu.getGyroX());
  file.print("; GyroY:");
  file.print(mpu.getGyroY());
  file.print("; GyroZ:");
  file.print(mpu.getGyroZ());
  file.print("; Pres:");
  file.print(bme.readPressure() / 100.0F);
  file.print("; Alt:");
  file.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  file.print("; Hum:");
  file.print(bme.readHumidity());
  file.print("; GsrAvg:");
  file.print(gsr_average);
  file.print("; ohm:");
  file.println(gsr_ohm);

  //  LEDLight();
  if (buttonState == 1) {
    // digitalWrite(LEDindicator, HIGH);
    pixels.setPixelColor(0, pixels.Color(50, 0, 0));
    pixels.show();
    // buzzerFst();
    // delay(200);
    // tone(buzzer, 3950);  // Send 1KHz sound signal...
    // delay(2000);         // ...for 1 sec
    // noTone(buzzer);      // Stop sound...
    delay(50);
  } else {
    // digitalWrite(LEDindicator, LOW);
    pixels.setPixelColor(0, pixels.Color(0, 0, 50));
    pixels.show();
    // buzzerSlw();
    delay(50);
  }

  //  counterCheck();
    // compare the buttonState to its previous state
    if (buttonState != lastButtonState) {
      // if the state has changed, increment the counter
      if (buttonState == HIGH) {
        // if the current state is HIGH then the button
        // wend from off to on:
        buttonPushCounter++;
        Serial.println("Started recording timestamp");
        Serial.print("number of button pushes:  ");
        Serial.println(buttonPushCounter);
      } else {
        // if the current state is LOW then the button
        // wend from on to off:
        //      buttonLog = 0;
      }
      // Delay a little bit to avoid bouncing
      delay(180); //ori = 50
    }
    // save the current state as the last state,
    //for next time through the loop
    lastButtonState = buttonState;
  
    // turns on the memory recording every two button pushes by
    // checking the modulo of the button push counter.
    // the modulo function gives you the remainder of
    // the division of two numbers:
  
    //  if (buttonPushCounter % 2 == 0) {
    if (buttonPushCounter % 2 == 1) {
      pixels.setPixelColor(0, pixels.Color(0, 50, 50));
      pixels.show();
      buttonLog = 1;
    } else {
      buttonLog = 0;
          Serial.println("TimeStamp recording stop");
      //idk what to put here.
    }
}


void startBuzz() {
  tone(buzzer, 3550);  // Send 1KHz sound signal...
  delay(500);
  noTone(buzzer);  // Stop sound...
  delay(300);
  tone(buzzer, 3750);  // Send 1KHz sound signal...
  delay(300);
  noTone(buzzer);  // Stop sound...
  delay(100);
  tone(buzzer, 4020);  // Send 1KHz sound signal...
  delay(400);
  noTone(buzzer);  // Stop sound...
  delay(50);
}

void buzzerFst() {
  tone(buzzer, 3950);  // Send 1KHz sound signal...
  delay(500);
  noTone(buzzer);  // Stop sound...
  delay(300);
}

void buzzerSlw() {
  for (int i = 0; i < 2; i++) {
    tone(buzzer, 3525);  // Send 1KHz sound signal...
    delay(400);
    noTone(buzzer);  // Stop sound...
    delay(280);
  }
}

///Memory option////////////

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = SPIFFS.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = SPIFFS.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2) {
  Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("- file renamed");
  } else {
    Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}