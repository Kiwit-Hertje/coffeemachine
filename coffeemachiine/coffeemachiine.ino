#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Fonts/FreeSans9pt7b.h>
#include "RTClib.h"

// Which button has been pressed
enum Button { power,
              brew,
              clean,
              none };

// The two actions that can be performed
enum Action { brewing,
              cleaning };

// Pins connected to the buttons
#define POWER 5
#define COFFEE 16
#define SELF_CLEANING 17

// Configuration of the screen
#define TFT_DC 2
#define TFT_CS 15
#define TFT_RESET 4

// Duration of the processes
#define COFFEE_DURATION 5000
#define CLEANING_DURATION 5000


// Position of items on the screen
const int timeX = 80;
const int timeY = 80;

const int barX = 10;
const int barY = 110;
const int barHeight = 30;
const int barWidth = ILI9341_TFTHEIGHT - ((10 + 16) * 2);
const int barSpacing = 4;
const int barRounding = 12;

const int brewingTextX = 100;
const int cleaningTextX = brewingTextX - 6;
const int textY = 170;

// tft screen
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RESET);

// buffer to prevent flickering of the screen
// set to heigth 175 in stead of 240. Setting it higher makes in realy slow. I don't know why.
GFXcanvas16 canvas(320, 175);


// reading the buttons

Button getButton() {
  if (digitalRead(POWER) == HIGH) {
    Serial.println("power");
    return power;
  } else if (digitalRead(COFFEE) == HIGH) {
    Serial.println("coffee");
    return brew;
  } else if (digitalRead(SELF_CLEANING) == HIGH) {
    Serial.println("clean");
    return clean;
  } else {
    return none;
  }
}


// displaying things on the screen

void show() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 320, 175);
}

void displayClear() {
  tft.fillScreen(ILI9341_BLACK);
}


void drawProgressBar(int progress) {
  canvas.drawRoundRect(barX, barY, ((100 * barWidth) / 100) + ((barSpacing + barRounding) * 2), barHeight, barRounding, ILI9341_DARKGREY);
  canvas.fillRoundRect(barX + barSpacing, barY + barSpacing, ((progress * barWidth) / 100) + ((barRounding)*2), barHeight - (barSpacing * 2), barRounding - (barSpacing * 2), ILI9341_WHITE);
}

void displayProgress(Action action, long currentTime, long endTime) {

  int progress = ((currentTime)*100) / endTime;
  canvas.fillScreen(ILI9341_BLACK);
  canvas.setCursor(timeX, timeY);
  TimeSpan ts = TimeSpan((endTime - currentTime) / 1000);
  canvas.printf("%02d:%02d until finished", ts.minutes(), ts.seconds());
  switch (action) {
    case brewing:
      canvas.setCursor(brewingTextX, textY);
      canvas.print("brewing");
      break;
    case cleaning:
      canvas.setCursor(cleaningTextX, textY);
      canvas.print("cleaning");
      break;
  }

  canvas.print(" ");
  canvas.print(progress);
  canvas.print("%");

  drawProgressBar(progress);
  show();
}

void displayFinished(Action action) {
  canvas.fillScreen(ILI9341_BLACK);
  switch (action) {
    case brewing:
      canvas.setCursor(brewingTextX, textY);
      canvas.print("brewing");
      break;
    case cleaning:
      canvas.setCursor(cleaningTextX, textY);
      canvas.print("cleaning");
      break;
  }

  canvas.print(" finished");
  drawProgressBar(100);
  show();
}



// The different states of the coffee machine. 

class State {
public:
  State() {}
  virtual void run(Button button) = 0;
};

void setState(State *newState);

void runState(Button button);


class OffState : public State {
public:
  void run(Button button);
};

class OnState : public State {
public:
  void run(Button button);
};

class CoffeeState : public State {
private:
  long _startTime;
public:
  CoffeeState(long startTime)
    : _startTime(startTime) {}
  void run(Button button);
};

class CleaningState : public State {
private:
  long _startTime;
public:
  CleaningState(long startTime)
    : _startTime(startTime) {}
  void run(Button button);
};

State *currentState;

void OffState::run(Button button) {
  displayClear();
  if (button == power) {
    setState(new OnState());
  }
}

void OnState::run(Button button) {
  switch (button) {
    case power:
      setState(new OffState());
      return;
    case brew:
      setState(new CoffeeState(millis()));
      return;
    case clean:
      setState(new CleaningState(millis()));
      return;
  }
}

void CoffeeState::run(Button button) {
  if (button == power) {
    setState(new OffState());
    return;
  }

  long currentTime = millis() - _startTime;
  if (currentTime >= COFFEE_DURATION) {
    displayFinished(brewing);
    setState(new OnState());
    return;
  } else {
    displayProgress(brewing, currentTime, COFFEE_DURATION);
  }
}

void CleaningState::run(Button button) {
  if (button == power) {
    setState(new OffState());
    return;
  }
  long currentTime = millis() - _startTime;
  if (currentTime >= COFFEE_DURATION) {
    displayFinished(cleaning);
    setState(new OnState());
    return;
  } else {
    displayProgress(cleaning, currentTime, COFFEE_DURATION);
  }
}

void runState(Button button) {
  currentState->run(button);
}

void setState(State *newState) {
  delete (currentState);
  currentState = (State *)newState;
}





void setup() {
  // setup the buttons
  pinMode(POWER, INPUT);
  pinMode(COFFEE, INPUT);
  pinMode(SELF_CLEANING, INPUT);
  Serial.begin(115200);
  // setup the screen
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  // load the font
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(ILI9341_WHITE);

  // start in the off state
  setState(new OffState());
}

void loop() {
  Button button = getButton();
  runState(button);
  delay(100);
}