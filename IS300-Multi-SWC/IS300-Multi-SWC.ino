#include <SPI.h>

// If defined, relay will toggle at specified interval
// #define DEBUG_RELAY_TOGGLE 1000

// If defined, volume will be turned up, then back down continually
// Numeric value defines how many presses per cycle
// #define DEBUG_VOLUME_ROLLERCOASTER 3

#define DEBUG_PRINT_READINGS false

/* * *
 * Pin Assignments
 * * */
const int PIN_CRUISE_SENSE = A0; // Read Cruse Control signal voltage
const int PIN_BUZZER_OUTPUT = 3; // Buzzer used to audibily indicate mode change
const int PIN_BYPASS_RELAY = 4; // Interrupts Cruise Control signal wire
const int PIN_SHIFT_SW_UP = 7; // Detect 'Up' shift button
const int PIN_SHIFT_SW_DN = 8; // Detecth 'Down' shift button
const int PIN_POT_CS = 10; // Potentiometer Chip Select
// POT MOSI = 11
// POT SCK = 13

/* * *
 * Constants
 * * */

enum RadioCommand {
  CMD_VOLUME_UP,
  CMD_VOLUME_DOWN,
  CMD_SEEK_PLUS,
  CMD_SEEK_MINUS,
  CMD_CHANGE_SRC,
  CMD_CALL_ANSWER,
  CMD_CALL_HANG_UP,
  CMD_VOICE_COMMAND,
};

// Bypass Relay & Switching
const unsigned long BYPASS_SWITCH_DEBOUNCE = 750;

const unsigned long BTN_SHORT_PRESS_MINIMUM = 110;
const unsigned long BTN_LONG_PRESS_MINIMUM = 450;

// Buzzer
const int BUZZ_TONE_HIGH = 100;
const int BUZZ_TONE_LOW = 50;
const int BUZZ_TONE_TIME = 250;

// Potentiometers
const int POT_SELECT_0 = 0x11; // Select Pot 0
const int POT_SELECT_1 = 0x12; // Select Pot 1

// Potentiometer shutdown
// When shutdown, terminal 'A' is open while terminals 'B' and 'W' are shorted
const int POT_SHUTDOWN_0 = 0x21;
const int POT_SHUTDOWN_1 = 0x22;

// Potentiometer resistance values
// These values correspond to values expected by a Pioneer radio (via W/R port)
const int RESISTANCE_SRC = 1200;
const int RESISTANCE_SEEK_PLUS = 8000;
const int RESISTANCE_SEEK_MINUS = 11250;
const int RESISTANCE_VOL_PLUS = 16000;
const int RESISTANCE_VOL_MINUS = 24000;

// Used as dummy value when shutting down digital potentiometer
const int POT_VAL_NO_BTN = 255;

// MCP42100 accepts 1 byte for the desired resistance value
// Divide the desired resistance (e.g. 1200 Ohm) by this value to receive an approximation of the corresponding byte value
const int POT_RESISTANCE_DIVISOR = 390;

// Number of milliseconds to present resistance value to radio
const int RADIO_CMD_DURATION = 250;

// Cruse control values (via Analog input)
// These are the expected values when cruise control is bypassed
const int CC_VAL_QUIESCENT = 722;
const int CC_VAL_UP = 195;
const int CC_VAL_DOWN = 356;
const int CC_VAL_CANCEL = 509;
const int CC_VAL_ENABLE = 0;

const float CC_PRECISION_MULTIPLIER = 0.05;

const float CC_BYPASS_DISABLED_MULTIPLIER = 1.0;
const float CC_BYPASS_DISABLED_OFFSET = 0;

enum CruseControlState {
  CC_UNKNOWN,
  CC_QUIESCENT,
  CC_UP,
  CC_DOWN,
  CC_CANCEL,
  CC_ENABLE,
};

// Shift button settings
const bool LONG_PRESS_AS_REPEAT = true;

/* * *
 * Global state
 * * */

// Bypass Relay & Switching
bool bypassEnabled = false;
int bothPressCount = 0;
unsigned long lastBypassTransitionTime = millis();

// Container for indicator tone
struct ToneIndication {
  unsigned long startAt;
  int tone1Val;
  int tone1Duration;
  int tone2Val;
  int tone2Duration;
} toneIndicator;

struct CruseControlReading {
  CruseControlState state;
  unsigned long since;
  bool fulfilled;
} cruseControlReading;

struct ShiftButtonReading {
  bool pressed;
  unsigned long since;
  bool fulfilled;
} shiftBtnUp, shiftBtnDown;

struct PotentiometerTimeoutRequest {
  unsigned long disableAfter;
  bool fulfilled;
} potTimeout0, potTimeout1;

void setup()
{
  Serial.begin(115200);

  pinMode(PIN_POT_CS, OUTPUT);
  pinMode(PIN_SHIFT_SW_UP, INPUT); 
  pinMode(PIN_SHIFT_SW_DN, INPUT);
  pinMode(PIN_BUZZER_OUTPUT, OUTPUT);
  pinMode(PIN_BYPASS_RELAY, OUTPUT);
  
  SPI.begin();
  delay(1);
  DigitalPotWrite(POT_SHUTDOWN_0, POT_VAL_NO_BTN);
  delay(1);
}

void loop() {
  int cruiseVal = analogRead(PIN_CRUISE_SENSE);
  bool upPressed = !digitalRead(PIN_SHIFT_SW_UP);
  bool downPressed = !digitalRead(PIN_SHIFT_SW_DN);

  #ifdef DEBUG_PRINT_READINGS
  Serial.print(cruiseVal, DEC);
  Serial.print('\t');  
  Serial.print(upPressed, HEX);
  Serial.print('\t');
  Serial.print(downPressed, HEX);
  Serial.print("\tBYPASS:");
  if (bypassEnabled) {
    Serial.print("ON");
  } else {
    Serial.print("OFF");
  }
  Serial.println();
  #endif

  serviceBypassDetection(upPressed, downPressed);
  serviceTone();
  bool recentBypassTransition = millis() < lastBypassTransitionTime + BYPASS_SWITCH_DEBOUNCE;
  if (bypassEnabled && !recentBypassTransition) {
    serviceCruseControl(cruiseVal);
    serviceShiftButton(upPressed, &shiftBtnUp, CMD_VOLUME_UP, CMD_SEEK_PLUS);
    serviceShiftButton(downPressed, &shiftBtnDown, CMD_VOLUME_DOWN, CMD_SEEK_MINUS);
  }
  servicePotentiometerTimeouts();
  
  digitalWrite(PIN_BYPASS_RELAY, bypassEnabled);


  #ifdef DEBUG_VOLUME_ROLLERCOASTER
  for(int i = 0; i < DEBUG_VOLUME_ROLLERCOASTER; i++) {
    SendRadioCommand(CMD_VOLUME_UP);
  }
  delay(2000);
  for(int i = 0; i < DEBUG_VOLUME_ROLLERCOASTER; i++) {
    SendRadioCommand(CMD_VOLUME_DOWN);
  }
  #endif

  #ifdef DEBUG_RELAY_TOGGLE
  if (millis() > lastBypassTransitionTime + DEBUG_RELAY_TOGGLE) {
    bypassEnabled = !bypassEnabled;
    lastBypassTransitionTime = millis();
    digitalWrite(PIN_BYPASS_RELAY, bypassEnabled);
  }
  #endif
}

void serviceCruseControl(int analogReading) {
  CruseControlState state = mapCruseControlState(analogReading);

  bool matchesPrevious = state == cruseControlReading.state;
  bool transitioningToQuiescent = state == CC_QUIESCENT && cruseControlReading.state != CC_QUIESCENT;

  if (matchesPrevious) {
    // Has it been long enough to consider this a long hold and thus fulfill right away?
    if (millis() > cruseControlReading.since + BTN_LONG_PRESS_MINIMUM) {
      mapCruiseActionToRadioAction(cruseControlReading.state, true);
      cruseControlReading.fulfilled = true;
    }

    return;
  } else if (transitioningToQuiescent && !cruseControlReading.fulfilled) {
    // Control was released, and wasn't already fulfilled
    // Verify the press was long enough
    if (millis() > cruseControlReading.since + BTN_SHORT_PRESS_MINIMUM) {
      mapCruiseActionToRadioAction(cruseControlReading.state, false);
      cruseControlReading.fulfilled = true;
    }
  } else {
      cruseControlReading.state = state;
      cruseControlReading.since = millis();
      cruseControlReading.fulfilled = false;
  }
}

void mapCruiseActionToRadioAction(CruseControlState state, bool longPress) {
  switch (state) {
    case CC_QUIESCENT:
      return;
    case CC_UP:
      if (longPress) {
        SendRadioCommand(CMD_CALL_ANSWER);
        break;
      }
        SendRadioCommand(CMD_SEEK_PLUS);
      break;
    case CC_DOWN:
      if (longPress) {
        SendRadioCommand(CMD_CALL_HANG_UP);
        break;
      }
        SendRadioCommand(CMD_SEEK_MINUS);
      break;
    case CC_CANCEL:
      if (longPress) {
        // FIXME: This is an ugly hack, but a convenient one
        serviceBypassDetection(true, true);
        serviceBypassDetection(true, true);
        break;
      }
      SendRadioCommand(CMD_CHANGE_SRC);
      break;
  }
}

void serviceShiftButton(bool pressed, ShiftButtonReading *prevState, RadioCommand shortAction, RadioCommand longAction) {
  if (pressed && prevState->pressed && !prevState->fulfilled) {
    if (millis() > prevState->since + BTN_LONG_PRESS_MINIMUM) {
      if (LONG_PRESS_AS_REPEAT) {
        SendRadioCommand(shortAction);
      } else {
        SendRadioCommand(longAction);
        prevState->fulfilled = true;
      }
    }
  } else if (!pressed && prevState->pressed && !prevState->fulfilled) {
    if (millis() > prevState->since + BTN_SHORT_PRESS_MINIMUM) {
      SendRadioCommand(shortAction);
      prevState->fulfilled = true;
    }
  } else {
      prevState->since = millis();
      prevState->pressed = pressed;
      prevState->fulfilled = false;
  }
}

CruseControlState mapCruseControlState(int analogReading) {
  if (analogReading == 0) {
    return CC_ENABLE;
  }

  float precisionOffset = float(analogReading) * CC_PRECISION_MULTIPLIER;
  if (!bypassEnabled) {
    precisionOffset *= CC_BYPASS_DISABLED_MULTIPLIER;
    precisionOffset += CC_BYPASS_DISABLED_OFFSET;
  }

  float lowerBounds = analogReading - precisionOffset;
  float upperBounds = analogReading + precisionOffset;

  #ifdef DEBUG_PRINT_READINGS
  Serial.print("Evaluating CC reading of ");
  Serial.print(analogReading, DEC);
  Serial.print(" with lower/upper bounds of ");
  Serial.print(lowerBounds, DEC);
  Serial.print("/");
  Serial.println(upperBounds, DEC);
  #endif

  if (isBetween(CC_VAL_QUIESCENT, lowerBounds, upperBounds)) {
    return CC_QUIESCENT;
  }
  if (isBetween(CC_VAL_CANCEL, lowerBounds, upperBounds)) {
    return CC_CANCEL;
  }
  if (isBetween(CC_VAL_UP, lowerBounds, upperBounds)) {
    return CC_UP;
  }
  if (isBetween(CC_VAL_DOWN, lowerBounds, upperBounds)) {
    return CC_DOWN;
  }
  return CC_UNKNOWN;
}

bool isBetween(int testValue, float lowerLimit, float upperLimit) {
  return testValue >= lowerLimit && testValue <= upperLimit;
}

void SendRadioCommand(RadioCommand cmd) {
  int resistanceValue = getResistanceForCommand(cmd);
  if (resistanceValue == 0) {
    Serial.print("Unable send command type ");
    Serial.println(cmd, HEX);
    return;
  }

  int mcpValue = calculatePotResistanceByte(resistanceValue);
  DigitalPotWrite(POT_SELECT_0, mcpValue);
  potTimeout0.disableAfter = millis() + RADIO_CMD_DURATION;
  potTimeout0.fulfilled = false;

}

void servicePotentiometerTimeouts() {
  if (!potTimeout0.fulfilled && millis() >= potTimeout0.disableAfter) {
      DigitalPotWrite(POT_SHUTDOWN_0, POT_VAL_NO_BTN);
  }
}

// Determine approximate byte value corresponding to desired resistance
int calculatePotResistanceByte(int resistanceOhms) {
  return resistanceOhms/POT_RESISTANCE_DIVISOR;
}

int getResistanceForCommand(RadioCommand cmd) {
  switch (cmd) {
    case CMD_VOLUME_UP:
      return RESISTANCE_VOL_PLUS;
    case CMD_VOLUME_DOWN:
      return RESISTANCE_VOL_MINUS;
    case CMD_SEEK_PLUS:
      return RESISTANCE_SEEK_PLUS;
    case CMD_SEEK_MINUS:
      return RESISTANCE_SEEK_MINUS;
    case CMD_CHANGE_SRC:
      return RESISTANCE_SRC;
    
    // These will need to be handled via Pot 1
    // We'll need a mechanism to indicate which Pot should present a given value
    case CMD_CALL_ANSWER:
    case CMD_CALL_HANG_UP:
    case CMD_VOICE_COMMAND:
    default: return 0;
  }
}


void DigitalPotWrite(int cmd, int val) {
  val = constrain(val, 0, 255);

  Serial.print("Writing ");
  Serial.print(val, DEC);
  Serial.println();

  digitalWrite(PIN_POT_CS, LOW);
  SPI.transfer(cmd);
  SPI.transfer(val);
  digitalWrite(PIN_POT_CS, HIGH);
}

// Returns whether bypass was changed
void serviceBypassDetection(bool upPressed, bool downPressed) {
  if (upPressed && downPressed) {
    bothPressCount++;
    if (bothPressCount >= 2 && (millis() > lastBypassTransitionTime + BYPASS_SWITCH_DEBOUNCE)) {
      bypassEnabled = !bypassEnabled;
      lastBypassTransitionTime = millis();
      bothPressCount = 0;

      /* Indicate transition */
      indicateTone(bypassEnabled);
    }
    return;
  }
  bothPressCount = 0;
}

void serviceTone() {
  if (millis() > toneIndicator.startAt + toneIndicator.tone1Duration + toneIndicator.tone2Duration) {
    /* Tone is over */
    digitalWrite(PIN_BUZZER_OUTPUT, 0);
    return;
  }
  if (millis() >= toneIndicator.startAt + toneIndicator.tone1Duration) {
    /* Time for secondary tone */
    analogWrite(PIN_BUZZER_OUTPUT, toneIndicator.tone2Val);
    return;
  }
  if (millis() >= toneIndicator.startAt) {
    /* Time for primary tone */
    analogWrite(PIN_BUZZER_OUTPUT, toneIndicator.tone1Val);
    return;
  }
}

void indicateTone(bool bypassActivated) {
    toneIndicator.startAt = millis();
    if (bypassActivated) {
      toneIndicator.tone1Val = BUZZ_TONE_LOW;
    toneIndicator.tone2Val = BUZZ_TONE_HIGH;
    } else {
      toneIndicator.tone1Val = BUZZ_TONE_HIGH;
      toneIndicator.tone2Val = BUZZ_TONE_LOW;
    }
    toneIndicator.tone1Duration = BUZZ_TONE_TIME;
    toneIndicator.tone2Duration = BUZZ_TONE_TIME;
}
