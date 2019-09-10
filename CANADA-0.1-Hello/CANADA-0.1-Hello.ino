const int TRIG_COUNT = 7;
const int triggerOutputs[TRIG_COUNT] = { 8, 6, 12, 4, 2, 3, 11 };

void setup() {
  Serial1.begin(115200);
  Serial1.println("Hello from CANADA1 [Made in California]");
  for (int i = 0; i < TRIG_COUNT; i++) {
    pinMode(triggerOutputs[i], OUTPUT);
  }
  Serial1.println("Initialization done.");
}

int currentTrigger = triggerOutputs[0];
void loop() {
  for(int i = 0; i < TRIG_COUNT; i++) {
    currentTrigger = triggerOutputs[i];
    Serial1.print("Trigger ");
    Serial1.print(currentTrigger);

    Serial1.print(" HIGH");
    digitalWrite(currentTrigger, HIGH);

    delay(100);

    Serial1.println("/LOW");
    digitalWrite(currentTrigger, LOW);

    delay(100);
    
  }
}
