#include <Keypad.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte buzzerPin = 11;
byte flatOpenerPin = 12;
byte houseOpenerPin = 10;
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

String readCode;

const bool twoClickMode = false; // if house door opens with one or two rings
int houseDoorState = 0;
unsigned long lastRing = 0;

enum UserState {
  PIN_ENTRY,
  MODE_TAN,
  MODE_TAN_ENTRY,
  MODE_FREE_ENTRY
};

int codeTries = 0;
unsigned long lastKeyPress = 0;
UserState currentKeypadState = PIN_ENTRY;
boolean keylessEntry = false;

const unsigned long counterResetTimeInSec = 3600;   // wrong pin counter
const unsigned long menuResetTimeInSec    = 60;     // menu exit counter
const unsigned long keylessResetTimeInSec = 36000;  // keyless mode cancel counter

const int approvedCodesCount = 5;
String approvedCodes[approvedCodesCount] = {"123456", "000000", "0123", "999999", "111111"};

const int tanCodeCount = 5;
String tanPinCodes[tanCodeCount] = {"3743467", "124558", "508694", "234498", "629543"};
bool activePinCodes[tanCodeCount] = {false, false, false, false, false};

void openHouseDoor();
void openFlatDoor();
boolean checkCode(String code);

/**
   Setup
   Set pins, set initial states and beep if done
*/
void setup() {
  pinMode(flatOpenerPin, OUTPUT);
  digitalWrite(flatOpenerPin, HIGH);
  pinMode(houseOpenerPin, OUTPUT);
  digitalWrite(houseOpenerPin, HIGH);
  //Serial.begin(9600);

  houseDoorState = 0;

  tone(1, 2000, 500);
}


/**
   Loop
   check house and flat door over and over.
*/
void loop() {
  checkHouseDoor();
  checkFlatDoor();
}


/**
   Check the house door (Bell downstairs)
*/
void checkHouseDoor() {
  int sensorValue = analogRead(A0);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);

  if (twoClickMode) {
    // ring two times in 1s to open the door
    if (voltage > 0.5) {
      if (houseDoorState = 2 && millis() - lastRing < 1000) {
        houseDoorState = 3; // second ring in time, open door
        openHouseDoor();
        houseDoorState = 0;
      } else {
        lastRing = millis(); // first ring, record time and set to state 1 (wait for voltage drop, same as in state 0)
        houseDoorState = 1;
      }
    }
    if (voltage < 0.1) {
      if (millis() - lastRing < 650 &&  houseDoorState == 3) {
        houseDoorState = 2; // after first ring, wait for second (state 2)
      } else {
        houseDoorState = 0; // took to long, go back to state 0
      }
    }
  } else {
    //one ring opens the door
    if (voltage > 0.5 && houseDoorState == 0) {
      houseDoorState = 1;
      openHouseDoor();
    }
    if (voltage < 0.1 && houseDoorState == 1) {
      houseDoorState = 0;
    }
  }
}

/**
   Check flat door.
   Check the keypad (and nfc if not disabled)
*/
void checkFlatDoor() {
  checkTimeouts();
  checkKeypad();
}

void checkTimeouts() {
  long timePassed = millis() - lastKeyPress;

  //if time for menu reset is up, go back to pin entry
  if ((timePassed > (menuResetTimeInSec * 1000)) && (currentKeypadState != PIN_ENTRY)) {
    resetKeypadState();
  }

  //if time for counter reset is up, reset codeTries to 0
  if ((timePassed > (counterResetTimeInSec * 1000)) && (codeTries != 0)) {
    // currentKeypadState = PIN_ENTRY; // (done above)
    codeTries = 0;
    resetKeypadState();
  }

  //deactivate keyless mode after given time
  if ((timePassed > (keylessResetTimeInSec * 1000)) && keylessEntry) {
    resetKeypadState();
    keylessEntry = false;
  }
}

/**
   Check Keypad input.
   Does have to happen w/o pause or button presses are not detected
*/
void checkKeypad() {
  char key = keypad.getKey();

  if(key){
    lastKeyPress = millis();
  }

  if (key == '*') {          // * -> reset buffer and state to pin entry
    tone(1, 2000, 300);
    resetKeypadState();
  } else if (key) {
    switch (currentKeypadState) {
      case PIN_ENTRY:
        // --------------------- PIN_ENTRY ---------------------
        if (key == '#') {   // # -> enter
          if (checkCode(readCode)) {
            tone(1, 2000, 500);
            openFlatDoor();
            codeTries = 0;
          } else {
            codeTries++;
            if (codeTries > 2) {
              tone(2, 2500, 90);
              delay(codeTries * 1000);
            }
            tone(3, 2500, 90);
          }
          readCode = "";
        } else if (key == 'A') {
          tone(1, 2000, 500);
          currentKeypadState = MODE_TAN;
          readCode = "";
        } else if (key == 'B') {
          tone(1, 2000, 500);
          currentKeypadState = MODE_FREE_ENTRY;
          readCode = "";
        } else if (key == 'C') {
          openHouseDoor();
        } else if (key == 'D') {
          if (keylessEntry) {
            tone(1, 2000, 500);
            openFlatDoor();
          } else {
            tone(3, 2500, 90);
          }
        } else if (key) {
          tone(1, 1800, 100);
          readCode += key;
        }
        break;
      case MODE_TAN:
        // --------------------- MODE_TAN ---------------------
        if (key == '#') {   // # -> enter
          if (checkCode(readCode)) {
            tone(1, 2000, 500);
            currentKeypadState = MODE_TAN_ENTRY;
            codeTries = 0;
          } else {
            codeTries++;
            if (codeTries > 2) {
              tone(2, 2500, 90);
              delay(codeTries * 1000);
            }
            tone(3, 2500, 90);
          }
          readCode = "";
        } else if (key == 'A') {
          tone(3, 2500, 90);
        } else if (key == 'B') {
          tone(3, 2500, 90);
        } else if (key == 'C') {
          tone(3, 2500, 90);
        } else if (key == 'D') {
          tone(3, 2500, 90);
        } else if (key) {
          tone(1, 1800, 100);
          readCode += key;
        }
        break;
      case MODE_TAN_ENTRY:
        // ------------------- MODE_TAN_ENTRY ------------------
        if (key == '#') {   // # -> enter
          for (int i = 0; i < tanCodeCount; i++) {
            if (!activePinCodes[i]) {
              tanPinCodes[i] = readCode;
              activePinCodes[i] = true;
              tone(1, 2000, 500);
              resetKeypadState();
              break;
            }
          }
          tone(3, 2500, 90);
          currentKeypadState = PIN_ENTRY;
        } else if (key == 'A') {
          tone(3, 2500, 90);
        } else if (key == 'B') {
          tone(3, 2500, 90);
        } else if (key == 'C') {
          for (int i = 0; i < tanCodeCount; i++) {
            tanPinCodes[i] = "";
            activePinCodes[i] = false;
          }
          tone(1, 2000, 500);
          resetKeypadState();
        } else if (key == 'D') {
          tone(3, 2500, 90);
        } else if (key) {
          tone(1, 1800, 100);
          readCode += key;
        }
        break;
      case MODE_FREE_ENTRY:
        // ------------------ MODE_FREE_ENTRY -----------------
        if (key == '#') {   // # -> enter
          if (checkCode(readCode)) {
            tone(1, 2000, 500);
            keylessEntry = !keylessEntry;
            resetKeypadState();
          } else {
            codeTries++;
            if (codeTries > 2) {
              tone(2, 2500, 90);
              delay(codeTries * 1000);
            }
            tone(3, 2500, 90);
          }
          readCode = "";
        } else if (key == 'A') {
          tone(3, 2500, 90);
        } else if (key == 'B') {
          tone(3, 2500, 90);
        } else if (key == 'C') {
          tone(3, 2500, 90);
        } else if (key == 'D') {
          tone(3, 2500, 90);
        } else if (key) {
          tone(1, 1800, 100);
          readCode += key;
        }
        break;
      default:
        tone(4, 2500, 90);
        break;
    }
  }
}

/**

*/
void resetKeypadState() {
  currentKeypadState = PIN_ENTRY;
  readCode = "";
  delay(300);
  tone(1, 2500, 90);
}

/**
   Check if the entered code is approcved
*/
boolean checkCode(String code) {
  if (currentKeypadState == PIN_ENTRY) {
    for (int i = 0; i < tanCodeCount; i++) {
      if (activePinCodes[i] && code == tanPinCodes[i]) {
        return true;
      }
    }
  } else {
    // intentionally blank
    // tans are not authorized
  }

  for (int i = 0; i < approvedCodesCount; i++) {
    if (approvedCodes[i] == code) {
      //Serial.println(approvedCodes[i]+"-"+code);
      return true;
    }
  }
  return false;
}


/**
   Open the upper door
*/
void openFlatDoor() {
  digitalWrite(flatOpenerPin, LOW);
  delay(500);
  digitalWrite(flatOpenerPin, HIGH);
}

/**
   Open the door below
*/
void openHouseDoor() {
  digitalWrite(houseOpenerPin, LOW);
  delay(350);
  digitalWrite(houseOpenerPin, HIGH);
  tone(1, 2000, 700);
}

/**
   Make a beep
   @param count: The times it beeps
   @param frequence: The frequence of the beep
   @param duration: The duration of a beep
*/
void tone(int count, int frequence, int duration) {
  for (int i = 0; i < count - 1; i++) {
    tone(buzzerPin, 100);
    delay(duration);
    noTone(buzzerPin);
    delay(100);
  }
  tone(buzzerPin, 100);
  delay(duration);
  noTone(buzzerPin);
}
