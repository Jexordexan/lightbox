// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Pins
int potPin = 2; // select the analog pin for the potentiometer
int doorPin = 6;
int buttonPin = 7; // select the digital pin for the button
int ledPin = 10;   // select the pin for the LED

// Potentiometer values
int potVal = 0;      // variable to store the value coming from the sensor
int lastDeciVal = 0; // variable to store the value coming from the sensor
int deciVal = 0;     // potentiometer value cast to 0-10

// Door switch values
int doorState = 0;

// Button values
int buttonState = 0;              // variable for reading the pushbutton status
int lastButtonState = 0;          // variable for storing the last pushbutton status
unsigned long buttonDownTime = 0; // millis

// Flags
boolean buttonClick = false;
boolean buttonHold = false;
boolean skipNextClick = false;

boolean useDefaultStrength = true;

// Timers and values
unsigned long startTime = 0; // millis
unsigned long elapsed = 0;   // millis
int cureDuration = 0;        // minutes
int lightStrength = 100;

// Screens
const int HOME = 0;
const int SET_TIME = 1;
const int SET_STR = 2;
const int DOOR_OPEN = 3;
const int CURING = 4;
const int PAUSED = 5;
const int STOP = 6;
const int DONE = 7;

int currentScreen = HOME;
int lastScreen = HOME;

int TimeIntervals[] = {5, 15, 30, 45, 60, 90, 120, 150, 180, 210, 240};

void setup()
{
  // declare the ledPin as an OUTPUT
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  turnOffLED();

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0, 0);
}

void loop()
{
  lcd.setCursor(0, 0);
  // Potentiometer
  potVal = analogRead(potPin); // read the value from the sensor
  deciVal = potVal / 100;

  // Door switch
  doorState = digitalRead(doorPin);

  // Button
  buttonState = digitalRead(buttonPin);

  // Button was pressed
  if (buttonState > lastButtonState)
  {
    buttonDownTime = millis();
  }

  if (buttonState == HIGH && (millis() - buttonDownTime) > 1000)
  {
    // Button is being held down
    // this stops a buttonClick from firing after the long press
    buttonHold = true;
    skipNextClick = true;
  }

  // Button was released
  if (buttonState < lastButtonState)
  {
    if (skipNextClick)
    {
      skipNextClick = false;
    }
    else
    {
      buttonClick = true;
    }
  }

  if (currentScreen != lastScreen)
  {
    lcd.clear();
  }
  lastScreen = currentScreen;

  // Router
  switch (currentScreen)
  {
  case HOME:
    HomeScreen();
    break;
  case SET_TIME:
    SetTimeScreen();
    break;
  case SET_STR:
    SetStrengthScreen();
    break;
  case CURING:
  case PAUSED:
    CuringScreen();
    break;
  case DOOR_OPEN:
    DoorOpenScreen();
    break;
  case STOP:
    StopScreen();
    break;
  case DONE:
    DoneScreen();
    break;
  default:
    ErrorScreen();
    break;
  }

  // Cleanup
  lastDeciVal = deciVal;
  lastButtonState = buttonState;
  buttonClick = false;
  buttonHold = false;
  delay(10);
}

void HomeScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("UV Light Box");

  lcd.setCursor(7, 1);
  lcd.print("Start ->");

  if (buttonClick)
  {
    currentScreen = SET_TIME;
  }
}

void SetTimeScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Cure time");

  lcd.setCursor(0, 1);

  int duration = TimeIntervals[deciVal];
  lcd.print(formatMinutes(duration));

  lcd.setCursor(8, 1);
  lcd.print("Next ->");

  if (buttonClick)
  {
    currentScreen = SET_STR;
    cureDuration = duration;
  }
}

void SetStrengthScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("UV strength");

  lcd.setCursor(0, 1);

  if (useDefaultStrength)
  {
    lcd.print("100%");
  }
  else
  {
    if (deciVal < 10)
    {
      lcd.print(" ");
    }

    lcd.print(deciVal * 10);
    lcd.print("%");
  }

  lcd.setCursor(9, 1);
  lcd.print("Run ->");

  if (buttonClick)
  {
    useDefaultStrength = true;
    lightStrength = deciVal * 10;

    if (doorState == HIGH)
    {
      currentScreen = DOOR_OPEN;
    }
    else
    {
      currentScreen = CURING;
      startTime = millis();
      turnOnLED();
    }
  }

  if (potVal != lastDeciVal)
  {
    useDefaultStrength = false;
  }
}

void CuringScreen()
{
  lcd.setCursor(0, 0);

  int elapsedMinutes = elapsed / 100; // (60 * 1000);
  int percentage = (100 * elapsedMinutes) / cureDuration;
  if (currentScreen == PAUSED)
  {
    lcd.print("PAUSED");
  }
  else
  {
    elapsed = (millis() - startTime);
  }

  printProgress();

  if (doorState == HIGH)
  {
    pauseProgress();
    currentScreen = DOOR_OPEN;
  }
  else if (buttonHold)
  {
    currentScreen = STOP;
    digitalWrite(ledPin, LOW);
  }
  else if (buttonClick)
  {
    if (currentScreen == PAUSED)
    {
      unpauseProgress();
    }
    else
    {
      pauseProgress();
    }
  }
  else if (percentage >= 100)
  {
    elapsed = 0;
    startTime = 0;
    currentScreen = DONE;
    turnOffLED();
  }
}

void pauseProgress()
{
  currentScreen = PAUSED;
  turnOffLED();
}

void unpauseProgress()
{
  currentScreen = CURING;
  startTime = millis() - elapsed;
  turnOnLED();
}

void DoorOpenScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("DOOR");

  printProgress();

  if (doorState == LOW)
  {
    unpauseProgress();
  }
}

void StopScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Cancel?");

  lcd.setCursor(8, 1);
  lcd.print("Yes ->");

  if (buttonClick)
  {
    currentScreen = SET_TIME;
  }
}

void DoneScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("DONE!");

  if (buttonClick)
  {
    currentScreen = HOME;
  }
}

void ErrorScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("ERROR!");
  lcd.print(currentScreen);
  lcd.print(buttonState);

  if (buttonClick)
  {
    currentScreen = HOME;
  }
}

// Functions

void turnOffLED()
{
  analogWrite(ledPin, 0);
}

void turnOnLED()
{
  analogWrite(ledPin, map(lightStrength, 0, 100, 0, 255));
}

void printProgress()
{
  int elapsedMinutes = elapsed / 100; // (60 * 1000);

  lcd.setCursor(7, 0);
  lcd.print(formatMinutes(elapsedMinutes));
  lcd.print('/');
  lcd.print(formatMinutes(cureDuration));

  int percentage = (100 * elapsedMinutes) / cureDuration;
  LCD_progress_bar(1, percentage, 0, 100);
}

String formatMinutes(int total)
{
  int hours = total / 60;
  int minutes = total - (60 * hours);
  String hourString = String(hours);
  String minString = String(minutes);

  if (minutes < 10)
  {
    minString = "0" + minString;
  }
  return hourString + ":" + minString;
}

void LCD_progress_bar(int row, int var, int minVal, int maxVal)
{
  int width = 16;
  int block = map(var, minVal, maxVal, 0, width);    // Block represent the current LCD space (modify the map setting to fit your LCD)
  int line = map(var, minVal, maxVal, 0, width * 5); // Line represent the theoretical lines that should be printed
  int bar = (line - (block * 5));                    // Bar represent the actual lines that will be printed

  /* LCD Progress Bar Characters, create your custom bars */

  byte bar1[8] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
  byte bar2[8] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
  byte bar3[8] = {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C};
  byte bar4[8] = {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E};
  byte bar5[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
  lcd.createChar(1, bar1);
  lcd.createChar(2, bar2);
  lcd.createChar(3, bar3);
  lcd.createChar(4, bar4);
  lcd.createChar(5, bar5);

  for (int x = 0; x < block; x++) // Print all the filled blocks
  {
    lcd.setCursor(x, row);
    lcd.write(byte(5));
  }

  lcd.setCursor(block, row); // Set the cursor at the current block and print the numbers of line needed
  if (bar != 0)
    lcd.write(bar);
  if (block == 0 && line == 0)
    lcd.write(' '); // Unless there is nothing to print, in this case show blank

  for (int x = width; x > block; x--) // Print all the blank blocks
  {
    lcd.setCursor(x, row);
    lcd.write(' ');
  }
}
