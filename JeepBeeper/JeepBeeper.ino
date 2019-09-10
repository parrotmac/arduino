const int inGreenRed = 2;
const int inBlueRed = 3;
const int inBlueYellow = 4;
const int inToyLED = 5;

#define NUM_SOUNDS 6
const int soundPins[NUM_SOUNDS] = { 6, 7, 8, 9, 10, 11 };

#define SOUND_BIRD 0
#define SOUND_CATT 1
#define SOUND_BOAT 2
#define SOUND_DUCK 3
#define SOUND_SONG 4
#define SOUND_DOGE 5

#define NUM_INPUTS 4
volatile long inputChangeMS[NUM_INPUTS] = {0, 0, 0, 0};


void greenRedChange() {
  inputChangeMS[0] = millis();
}

void blueRedChange() {
  inputChangeMS[1] = millis();
}

void blueYellowChange() {
  inputChangeMS[2] = millis();
}

void toyLEDChange() {
  inputChangeMS[3] = millis();
}

void setup() {
  pinMode(inGreenRed, INPUT);
  pinMode(inBlueRed, INPUT);
  pinMode(inBlueYellow, INPUT);
  pinMode(inToyLED, INPUT);

  
  attachInterrupt(digitalPinToInterrupt(inGreenRed), greenRedChange, FALLING);
  attachInterrupt(digitalPinToInterrupt(inBlueRed), blueRedChange, FALLING);
  attachInterrupt(digitalPinToInterrupt(inBlueYellow), blueYellowChange, FALLING);
  attachInterrupt(digitalPinToInterrupt(inToyLED), toyLEDChange, FALLING);

  for(int i = 0; i < NUM_SOUNDS; i++) {
    pinMode(soundPins[i], OUTPUT);
  }
  pinMode(LED_BUILTIN, OUTPUT);
}

void toggleOutputPin(int pinNum, bool high) {
  switch(pinNum) {
    case 0:
    digitalWrite(soundPins[SOUND_SONG], high);
    break;
    
    case 1:
    digitalWrite(soundPins[SOUND_CATT], high);
    break;

    case 2:
    digitalWrite(soundPins[SOUND_DOGE], high);
    break;

    case 3:
    // LED from toy. Don't do anything else for now
    break;

    default:
    break;
  }
}

void loop() {

  for(int i = 0; i < NUM_INPUTS; i++) {
    if(inputChangeMS[i] > 0) {
      if(inputChangeMS[i] + 500 < millis()) {
        // Turn pin off
        toggleOutputPin(i, LOW);
        inputChangeMS[i] = 0;
      } else {
        // Turn pin on
        toggleOutputPin(i, HIGH);
      }
      
    }
  }
  
}

