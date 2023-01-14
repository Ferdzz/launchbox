#include <LiquidCrystal.h>
#include "pitches.h"  // Used for buzzer tones
#include "arduino-timer.h"
#include "Talkie.h"
#include "Vocab_Special.h"   // Pauses
#include "Vocab_US_Large.h"  // Phonetic alphabet & most wordage
#include "Vocab_US_Acorn.h"  // Word completions

// Language definitions. Pauses at the start of words are to give time to initalize without cutting words
const uint8_t *spCALLSIGN[] = { spPAUSE2, sp4_VICTOR, sp4_ALPHA, sp3_TWO, sp4_FOXTROT, sp4_ECHO, sp4_ZULU };
const int spCALLSIGN_SIZE = sizeof spCALLSIGN / sizeof spCALLSIGN[0];
const uint8_t *spREADY[] = { spPAUSE2, sp2_READY, spPAUSE2, sp2_PRESS, sp2_ZERO, sp4_TO, sp2_START };
const int spREADY_SIZE = sizeof spREADY / sizeof spREADY[0];
const uint8_t *spCONTINUITY_CHECK_BEGIN[] = { spPAUSE2, sp2_CAUTION, spPAUSE2, sp2_START, spa__ING, sp4_IGNITION, sp2_CIRCUIT, sp2_TEST };
const int spCONTINUITY_CHECK_BEGIN_SIZE = sizeof spCONTINUITY_CHECK_BEGIN / sizeof spCONTINUITY_CHECK_BEGIN[0];
const uint8_t *spCONTINUITY_CHECK_SUCCESS[] = { spPAUSE2, sp4_IGNITION, sp2_CIRCUIT, sp2_TEST, spPAUSE1, sp2_CHECK };
const int spCONTINUITY_CHECK_SUCCESS_SIZE = sizeof spCONTINUITY_CHECK_SUCCESS / sizeof spCONTINUITY_CHECK_SUCCESS[0];
const uint8_t *spCONTINUITY_CHECK_ERROR[] = { spPAUSE2, sp4_IGNITION, sp2_CIRCUIT, sp2_TEST, spPAUSE1, sp4_FAILURE };
const int spCONTINUITY_CHECK_ERROR_SIZE = sizeof spCONTINUITY_CHECK_ERROR / sizeof spCONTINUITY_CHECK_ERROR[0];
const uint8_t *spABORT_LAUNCH[] = { spPAUSE2, sp4_NO, sp2_GO, spPAUSE2, spPAUSE2, sp2_ABORT, spa__ING, sp5_LAUNCH, spPAUSE2, spPAUSE2, sp2_SAFE, sp4_TO, sp5_APPROACH };
const int spABORT_LAUNCH_SIZE = sizeof spABORT_LAUNCH / sizeof spABORT_LAUNCH[0];
const uint8_t *spLAUNCH_CODE[] = { spPAUSE2, sp2_ENTER, sp5_LAUNCH, sp4_KEY, sp4_TO, sp5_IGNITE, sp5_ENGINE };
const int spLAUNCH_CODE_SIZE = sizeof spLAUNCH_CODE / sizeof spLAUNCH_CODE[0];
const uint8_t *spLAUNCH_CODE_SUCCESS[] = { spPAUSE2, sp5_LAUNCH, sp4_KEY, sp2_CHECK };
const int spLAUNCH_CODE_SUCCESS_SIZE = sizeof spLAUNCH_CODE_SUCCESS / sizeof spLAUNCH_CODE_SUCCESS[0];
const uint8_t *spLAUNCH_CODE_FAILURE[] = { spPAUSE2, sp5_LAUNCH, sp4_KEY, sp4_FAILURE };
const int spLAUNCH_CODE_FAILURE_SIZE = sizeof spLAUNCH_CODE_FAILURE / sizeof spLAUNCH_CODE_FAILURE[0];
const uint8_t *spSTARTING_LAUNCH_COUNTDOWN[] = { spPAUSE2, sp2_START, spa__ING, sp4_IGNITION, sp3_IN, sp2_FIVE, sp2_SECONDS, spPAUSE2, spPAUSE2, sp4_HOLD, sp2_NUMBER, sp4_TO, sp4_ABORT };
const int spSTARTING_LAUNCH_COUNTDOWN_SIZE = sizeof spSTARTING_LAUNCH_COUNTDOWN / sizeof spSTARTING_LAUNCH_COUNTDOWN[0];

// - Pin definitions
#define PIN_TRIGGER_LAUNCH 2
#define PIN_BUZZER 3
#define PIN_CONTINUITY_CHECK 4
#define PIN_PTT_TRANSMIT 5
#define PIN_DTMF_Q1 A0
#define PIN_DTMF_Q2 A1
#define PIN_DTMF_Q3 A2
#define PIN_DTMF_Q4 A3
#define PIN_DTMF_STQ A4     // Represents data ready
#define PIN_RANDOM_SEED A5  // Unconnected pin used as random seed

#define PIN_LCD_RS 13
#define PIN_LCD_EN 12
#define PIN_LCD_D4 9
#define PIN_LCD_D5 8
#define PIN_LCD_D6 7
#define PIN_LCD_D7 6
#define LCD_COLUMN_COUNT 16
#define LCD_ROW_COUNT 2

Talkie voice;  // It is implied that this uses pin 3 and pin 11
Timer<10> timer;

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

int launchCode;

// - Arduino setup

// TODO: All delays in this code should be replaced with Timer "async" calls
void setup() {
  Serial.begin(9600);

  pinMode(PIN_TRIGGER_LAUNCH, OUTPUT);
  digitalWrite(PIN_TRIGGER_LAUNCH, LOW);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_CONTINUITY_CHECK, INPUT_PULLUP);
  pinMode(PIN_PTT_TRANSMIT, OUTPUT);
  pinMode(PIN_DTMF_Q1, INPUT);
  pinMode(PIN_DTMF_Q2, INPUT);
  pinMode(PIN_DTMF_Q3, INPUT);
  pinMode(PIN_DTMF_Q4, INPUT);
  pinMode(PIN_DTMF_STQ, INPUT);

  lcd.begin(LCD_COLUMN_COUNT, LCD_ROW_COUNT);

  randomSeed(analogRead(PIN_RANDOM_SEED));
  launchCode = random(1000, 9999);

  lcd.print("Launch code: ");
  lcd.setCursor(0, 1);
  lcd.print(launchCode);

  delay(3000);
  do {
    delay(1000);
    // Transmit callsign & startup message
    transmit(spCALLSIGN, spCALLSIGN_SIZE);
    transmit(spREADY, spREADY_SIZE);
  } while (readDTMFInput() != 0);  // Wait for a DTMF signal of 0 before starting

  delay(1500);

  if (checkBlastContinuity()) {
    delay(1500);
    if (checkLaunchCode()) {
      triggerLaunch();
    } else {
      abortLaunch();
    }
  } else {
    abortLaunch();
  }
}

void loop() {
  timer.tick();
}

// - Launch flow

bool checkBlastContinuity() {
  transmit(spCONTINUITY_CHECK_BEGIN, spCONTINUITY_CHECK_BEGIN_SIZE);
  delay(1000);

  bool continuity = digitalRead(PIN_CONTINUITY_CHECK);
  if (continuity == LOW) {  // We expect continuity to ground
    transmit(spCONTINUITY_CHECK_SUCCESS, spCONTINUITY_CHECK_SUCCESS_SIZE);
    return true;
  } else {
    transmit(spCONTINUITY_CHECK_ERROR, spCONTINUITY_CHECK_ERROR_SIZE);
    return false;
  }
}

bool checkLaunchCode() {
  // TODO: This code can be cleaned up
  transmit(spLAUNCH_CODE, spLAUNCH_CODE_SIZE);

  int digit1 = (launchCode / 1000) % 10;
  int digit2 = (launchCode / 100) % 10;
  int digit3 = (launchCode / 10) % 10;
  int digit4 = (launchCode / 1) % 10;
  if (readDTMFInput() != digit1) {
    delay(1500);
    transmit(spLAUNCH_CODE_FAILURE, spLAUNCH_CODE_FAILURE_SIZE);
    return false;
  }
  if (readDTMFInput() != digit2) {
    delay(1500);
    transmit(spLAUNCH_CODE_FAILURE, spLAUNCH_CODE_FAILURE_SIZE);
    return false;
  }
  if (readDTMFInput() != digit3) {
    delay(1500);
    transmit(spLAUNCH_CODE_FAILURE, spLAUNCH_CODE_FAILURE_SIZE);
    return false;
  }
  if (readDTMFInput() != digit4) {
    delay(1500);
    transmit(spLAUNCH_CODE_FAILURE, spLAUNCH_CODE_FAILURE_SIZE);
    return false;
  }

  return true;
}
void triggerLaunch() {
  transmit(spSTARTING_LAUNCH_COUNTDOWN, spSTARTING_LAUNCH_COUNTDOWN_SIZE);

  delay(1000);
  // Every second, check if DTMF data is received, if any then abort
  transmit(sp2_FIVE);
  checkForAbortAndDelay();
  transmit(sp2_FOUR);
  checkForAbortAndDelay();
  transmit(sp2_THREE);
  checkForAbortAndDelay();
  transmit(sp2_TWO);
  checkForAbortAndDelay();
  transmit(sp2_ONE);
  checkForAbortAndDelay();
  transmit(sp4_IGNITION);
  digitalWrite(PIN_TRIGGER_LAUNCH, HIGH);
  delay(3000);
  digitalWrite(PIN_TRIGGER_LAUNCH, LOW);
}

bool checkForAbortAndDelay() {
  long start = millis();
  while (millis() < start + 900) {
    if (digitalRead(PIN_DTMF_STQ) == HIGH) {
      abortLaunch();
    }
  }
}

void abortLaunch() {
  transmit(spABORT_LAUNCH, spABORT_LAUNCH_SIZE);
  lcd.clear();
  lcd.home();
  lcd.print("Launch aborted");

  abort();
}

// - Utility

void transmit(uint8_t **words, int wordCount) {
  Serial.print("Transmitting ");
  Serial.print(wordCount);
  Serial.println(" words");

  enablePTT();
  for (int i = 0; i < wordCount; i++) {
    voice.say(words[i]);
  }
  disablePTT();
}

void transmit(uint8_t *word) {
  enablePTT();
  voice.say(word);
  disablePTT();
}

void enablePTT() {
  digitalWrite(PIN_PTT_TRANSMIT, HIGH);
  // Ensure the relay has time to activate
  delay(500);
}

void disablePTT() {
  digitalWrite(PIN_PTT_TRANSMIT, LOW);
}

/// Returns the DTMF digit read. Blocking until data is available and back to unavailable
uint8_t readDTMFInput() {
  // Block until data is available
  while (digitalRead(PIN_DTMF_STQ) == LOW) {
    delay(15);
  }

  uint8_t numberPressed = (0x00 | (digitalRead(PIN_DTMF_Q1) << 0) | (digitalRead(PIN_DTMF_Q2) << 1) | (digitalRead(PIN_DTMF_Q3) << 2) | (digitalRead(PIN_DTMF_Q4) << 3));
  if (numberPressed == 10) {
    numberPressed = 0;  // Bit output for DTMF 0 is 10 but it's easier to handle it as 0
  }

  // Block before returning until the data is not transmitted anymore. This avoids successive calls to readDTMFInput returning the same value if the transmission isn't finished
  while (digitalRead(PIN_DTMF_STQ) == HIGH) {
    delay(5);
  }

  return numberPressed;
}
