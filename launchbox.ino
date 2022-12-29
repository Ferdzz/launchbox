#include "pitches.h" // Used for buzzer tones
#include "arduino-timer.h"

// enum State {
//   TRIGGER_LAUNCH,
//   POST_LAUNCH
// };

// enum State state = TRIGGER_LAUNCH;

const uint8_t PIN_TRIGGER_LAUNCH = 2;
const uint8_t PIN_BUZZER = 3;
const uint8_t PIN_CONTINUITY_CHECK = 4;

Timer<10> timer;

void setup() {
  //start serial connection
  Serial.begin(9600);

  pinMode(PIN_CONTINUITY_CHECK, INPUT_PULLUP);
  pinMode(PIN_TRIGGER_LAUNCH, OUTPUT);

  if (checkBlastContinuity()) {
    //triggerLaunch();
  }
}

void loop() {
  //checkBlastContinuity();
  timer.tick();
}

bool checkBlastContinuity() {
  // 1 means continuity, 0 means no continuity
  bool continuity = !digitalRead(PIN_CONTINUITY_CHECK);
  if (continuity) {
    tone(PIN_BUZZER, NOTE_C4, 250);
    delay(250);
    tone(PIN_BUZZER, NOTE_C5, 250);
    delay(250);
    tone(PIN_BUZZER, NOTE_C6, 250);
    // TODO: Transmit continuity check pass
    delay(1000);
    triggerLaunch();
  } else {
    tone(PIN_BUZZER, NOTE_C5, 250);
    delay(250);
    tone(PIN_BUZZER, NOTE_C4, 250);
    delay(250);
    tone(PIN_BUZZER, NOTE_C3, 250);
    // TODO: Transmit continuity check fail & abort
  }
  return continuity;
}

void triggerLaunch() {
  startIntervalBuzzer();

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

Timer<>::Task buzzerTask;
void startIntervalBuzzer() {
  // Buzzes for .75 second every second
  buzzerTask = timer.every(1000, [] { tone(PIN_BUZZER, NOTE_C3, 750); });
}

void stopIntervalBuzzer() {
  timer.cancel(buzzerTask);
}
