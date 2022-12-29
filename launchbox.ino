#include "pitches.h" // Used for buzzer tones
#include "arduino-timer.h"
#include "Talkie.h"
#include "Vocab_Special.h" // Pauses
#include "Vocab_US_Large.h" // Phonetic alphabet & most wordage
#include "Vocab_US_Acorn.h" // Word completions

// enum State {
//   TRIGGER_LAUNCH,
//   POST_LAUNCH
// };

// enum State state = TRIGGER_LAUNCH;

// Language defined
const uint8_t *spCALLSIGN[] = { spPAUSE2, sp4_VICTOR, sp4_ALPHA, sp3_TWO, sp4_FOXTROT, sp4_ECHO, sp4_ZULU };
const int spCALLSIGN_SIZE = sizeof spCALLSIGN / sizeof spCALLSIGN[0];
const uint8_t *spCONTINUITY_CHECK_BEGIN[] = { spPAUSE2, sp2_CAUTION, spPAUSE1, sp2_START, spa__ING, sp4_IGNITION, sp2_CIRCUIT, sp2_TEST };
const int spCONTINUITY_CHECK_BEGIN_SIZE = sizeof spCONTINUITY_CHECK_BEGIN / sizeof spCONTINUITY_CHECK_BEGIN[0];
const uint8_t *spCONTINUITY_CHECK_SUCCESS[] = { spPAUSE2, sp4_IGNITION, sp2_CIRCUIT, sp2_TEST, spPAUSE1, sp2_CHECK };
const int spCONTINUITY_CHECK_SUCCESS_SIZE = sizeof spCONTINUITY_CHECK_SUCCESS / sizeof spCONTINUITY_CHECK_SUCCESS[0];
const uint8_t *spCONTINUITY_CHECK_ERROR[] = { spPAUSE2, sp4_IGNITION, sp2_CIRCUIT, sp2_TEST, spPAUSE1, sp4_FAILURE };
const int spCONTINUITY_CHECK_ERROR_SIZE = sizeof spCONTINUITY_CHECK_ERROR / sizeof spCONTINUITY_CHECK_ERROR[0];
const uint8_t *spABORT_LAUNCH[] = { spPAUSE2, sp4_NO, sp2_GO, spPAUSE2, sp2_ABORT, spa__ING, sp5_LAUNCH };
const int spABORT_LAUNCH_SIZE = sizeof spABORT_LAUNCH / sizeof spABORT_LAUNCH[0];


#define PIN_TRIGGER_LAUNCH 2
#define PIN_BUZZER 3
#define PIN_CONTINUITY_CHECK 4
#define PIN_PTT_TRANSMIT 5

Talkie voice; // It is implied that this uses pin 3 and pin 11
Timer<10> timer;

void setup() {
  //start serial connection
  Serial.begin(9600);

  pinMode(PIN_TRIGGER_LAUNCH, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_CONTINUITY_CHECK, INPUT_PULLUP);
  pinMode(PIN_PTT_TRANSMIT, OUTPUT);

  // Transmit callsign & startup message
  transmit(spCALLSIGN, spCALLSIGN_SIZE);

  // TODO: Check blast in timer looping to avoid all this in setup
  if (!checkBlastContinuity()) {
    return;
  }
}

void loop() {
  //checkBlastContinuity();
  timer.tick();
}

bool checkBlastContinuity() {
  transmit(spCONTINUITY_CHECK_BEGIN, spCONTINUITY_CHECK_BEGIN_SIZE);
  delay(1000);

  // 1 means continuity, 0 means no continuity
  bool continuity = !digitalRead(PIN_CONTINUITY_CHECK);
  if (continuity == HIGH) {
    tone(PIN_BUZZER, NOTE_C4, 250);
    delay(250);
    tone(PIN_BUZZER, NOTE_C5, 250);
    delay(250);
    tone(PIN_BUZZER, NOTE_C6, 250);
    
    transmit(spCONTINUITY_CHECK_SUCCESS, spCONTINUITY_CHECK_SUCCESS_SIZE);

    delay(1000);
    triggerLaunch();
  } else {
    tone(PIN_BUZZER, NOTE_C5, 250);
    delay(250);
    tone(PIN_BUZZER, NOTE_C4, 250);
    delay(250);
    tone(PIN_BUZZER, NOTE_C3, 250);

    transmit(spCONTINUITY_CHECK_ERROR, spCONTINUITY_CHECK_ERROR_SIZE);
    transmit(spABORT_LAUNCH, spABORT_LAUNCH_SIZE);
  }
  return continuity;
}

void triggerLaunch() {
  startIntervalBuzzer();

  // { sp5_IGNITE, sp5_ENGINE, }

  // Wait 5 seconds & trigger the relay for 3 seconds
  timer.in(5000, [] {
    digitalWrite(PIN_TRIGGER_LAUNCH, HIGH);
    stopIntervalBuzzer();
    tone(PIN_BUZZER, NOTE_C4, 1000);
    timer.in(3000, [] {
      digitalWrite(PIN_TRIGGER_LAUNCH, LOW);
    });
  });
}

void loopPostLaunch() {
  // Transmit successful launch, shutdown
}

void transmit(uint8_t **words, int wordCount) {
  Serial.print("Transmitting ");
  Serial.print(wordCount);
  Serial.println(" words");

  digitalWrite(PIN_PTT_TRANSMIT, HIGH);
  for (int i = 0; i < wordCount; i++) {
    voice.say(words[i]);
  }
  digitalWrite(PIN_PTT_TRANSMIT, LOW);
}

Timer<>::Task buzzerTask;
void startIntervalBuzzer() {
  // Buzzes for .75 second every second
  buzzerTask = timer.every(1000, [] { tone(PIN_BUZZER, NOTE_C3, 750); });
}

void stopIntervalBuzzer() {
  timer.cancel(buzzerTask);
}
