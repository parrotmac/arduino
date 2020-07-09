const int redLedPin = 23;
const int greenLedPin = 22;
const int blueLedPin = 21;

// setting PWM properties
const int freq = 5000;
const int redLedChannel = 0;
const int greenLedChannel = 1;
const int blueLedChannel = 2;
const int resolution = 8;
const int ledDelay = 15;
 
void setup(){
  ledcSetup(redLedChannel, freq, resolution);
  ledcSetup(greenLedChannel, freq, resolution);
  ledcSetup(blueLedChannel, freq, resolution);
  
  ledcAttachPin(redLedPin, redLedChannel);
  ledcAttachPin(greenLedPin, greenLedChannel);
  ledcAttachPin(blueLedPin, blueLedChannel);
}
 
void loop(){
  // Increase Red
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){   
    ledcWrite(redLedChannel, dutyCycle);
    delay(ledDelay);
  }
  
  // Decrease Blue
  for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
    ledcWrite(blueLedChannel, dutyCycle);   
    delay(ledDelay);
  }

  // Increase Green
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){   
    ledcWrite(greenLedChannel, dutyCycle);
    delay(ledDelay);
  }

  // Decrease Red
  for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
    ledcWrite(redLedChannel, dutyCycle);   
    delay(ledDelay);
  }
  
  // Increase Blue
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){   
    ledcWrite(blueLedChannel, dutyCycle);
    delay(ledDelay);
  }
  
  // Decrease Green
  for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
    ledcWrite(greenLedChannel, dutyCycle);   
    delay(ledDelay);
  }
  
}
