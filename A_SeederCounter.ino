#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Bounce2.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define RESTART_BUTTON A3
#define HALL_SENSOR PD2

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Bounce debouncerRestart = Bounce();

// Calibration
float seederWidth = 2.5;  // in meter
float area = 8;           // in ar
float impulsPerArea = 140; 

// Impuls
unsigned long currentImpuls; // Impuls from hall sensor

// For program uses
float currentArea;                              // Area calculated from impuls
float currentDisplayArea;                       // Area displayed on lcd
float totalArea;                                // Total area displayed on lcd (stored in memory)
float distance = (area*100)/seederWidth;        // Distance calculated from area and seederWidth
float meterPerImpuls = distance/impulsPerArea;  // Meter per one impuls

void setup(void) {

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  debouncerRestart.attach(RESTART_BUTTON, INPUT_PULLUP);
  debouncerRestart.interval(25);
  totalArea = (float) EEPROMReadlong(9)/100;
  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR), increaseImpuls, FALLING);
  pinMode(HALL_SENSOR, INPUT_PULLUP);
}

void loop(void) {
  checkButtons();
  calculateArea();
  displayArea(currentArea, totalArea);
}

void displayArea(float currentArea, float totalArea) {
  display.clearDisplay();
  
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(40, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.print(currentDisplayArea,2);
  display.setCursor(0, 40);     // Start at top-left corner
  display.setTextSize(1); 
  display.setCursor(10, 25);     // Start at top-left corner
  display.print("Razem ");
  display.setCursor(55, 25);     // Start at top-left corner
  display.print(totalArea);
  display.display();
}
void calculateArea() {
  currentArea = currentImpuls * meterPerImpuls * seederWidth * 0.0001;
  float difference = currentArea-currentDisplayArea;
  if(difference>=0.01) {
    currentDisplayArea = currentArea;
    totalArea += difference;
    EEPROMWritelong(9, totalArea*100);
  }
}

void checkButtons() {
  debouncerRestart.update();
  if (debouncerRestart.fell()) { // Call code if button transitions from HIGH to LOW
    currentImpuls = 0;
    currentArea = 0;
    currentDisplayArea = 0;
  }
}

void increaseImpuls() {
  currentImpuls++;
}

void EEPROMWritelong(int address, long value) {
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address) {
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
