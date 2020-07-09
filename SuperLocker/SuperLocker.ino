#include "FPS_GT511C3.h" //the fps (fingerprint scanner) library

#define BUFFER_SIZE 40
#define FPS_ENABLED 1;

FPS_GT511C3 fps(10, 11); //RX, TX
// FPS_GT511C3 fps(A6, A7); //RX, TX

/*
    FPS Params
*/
int verifiedUserID = 0;
long verifiedUserAt = 0;
const int verificationMaxAge = 5000; // After a user is verified, consider them verified for the next 5 seconds
long flashErrorUntil = 0;
long lastFlashTransitionTime = 0;
bool flashState = false;
const int flashDuration = 100;

void fpsInit() {
    fps.UseSerialDebug = false; //set to true for fps debugging through serial
    fps.Open();
    fps.SetLED(true);
}

void fingerprintVerificationLoop() {
    if (flashErrorUntil > millis()) { // Within allowed flashing time window
        if (lastFlashTransitionTime == 0 || lastFlashTransitionTime + flashDuration < millis()) {

            fps.SetLED(flashState);
            flashState = !flashState;

            lastFlashTransitionTime = millis();
        }
    } else {
        fps.SetLED(true);
        lastFlashTransitionTime = 0;
        flashErrorUntil = 0;
    }

    if (verifiedUserAt + verificationMaxAge < millis()) {
        verifiedUserID = 0;
        return;
    }
}

bool fingerDetected() {
    return fps.IsPressFinger();
}

int getFingerprintUserID() {
    /*
    -1 = Unathorized
    > 0 = User ID
    */
  fps.CaptureFinger(false); //captures the finger for identification
  int id = fps.Identify1_N(); //identifies print and stores the id
//   Serial.print("read user ");
    // Serial.println(id, DEC);
  if (id < 200) {
      return id;
  }
  return -1;
}

void enrollFingerprint()  {
    // find open enroll id
	int enrollid = 0;
	bool usedid = true;
	while (usedid == true)
	{
		usedid = fps.CheckEnrolled(enrollid);
		if (usedid==true) enrollid++;
	}
	fps.EnrollStart(enrollid);

	// enroll
	Serial.print("Press finger to Enroll #");
	Serial.println(enrollid);
	while(fps.IsPressFinger() == false) delay(100);
	bool bret = fps.CaptureFinger(true);
	int iret = 0;
	if (bret != false)
	{
        for(int i = 0; i < 4; i++) { fps.SetLED(!i % 2 == 0); delay(50); }
		Serial.println("Remove finger");
		fps.Enroll1(); 
		while(fps.IsPressFinger() == true) delay(100);
		Serial.println("Press same finger again");
		while(fps.IsPressFinger() == false) delay(100);
		bret = fps.CaptureFinger(true);
		if (bret != false)
		{
            for(int i = 0; i < 4; i++) { fps.SetLED(!i % 2 == 0); delay(50); }
			Serial.println("Remove finger");
			fps.Enroll2();
			while(fps.IsPressFinger() == true) delay(100);
			Serial.println("Press same finger yet again");
			while(fps.IsPressFinger() == false) delay(100);
			bret = fps.CaptureFinger(true);
			if (bret != false)
			{
                for(int i = 0; i < 4; i++) { fps.SetLED(!i % 2 == 0); delay(50); }
				Serial.println("Remove finger");
				iret = fps.Enroll3();
				if (iret == 0)
				{
					Serial.println("Enrolling Successful");
				}
				else
				{
					Serial.print("Enrolling Failed with error code:");
					Serial.println(iret);
				}
			}
			else Serial.println("Failed to capture third finger");
		}
		else Serial.println("Failed to capture second finger");
	}
	else Serial.println("Failed to capture first finger");
}

/*
    Solenoid
*/

int unlockDurationMilliseconds = 0;
long lastUnlockAt = 0;
const int unlockSolenoidPin = 13;

void setupSolenoid() {
    pinMode(unlockSolenoidPin, OUTPUT);
    setLocked();
}

void setUnlocked() {
    digitalWrite(unlockSolenoidPin, HIGH);
}

void setLocked() {
    digitalWrite(unlockSolenoidPin, LOW);
}

/*
    Call periodically to unlock or re-lock
*/
void solenoidLoop() {
    if (lastUnlockAt + unlockDurationMilliseconds > millis()) {
        return;
    }
    setLocked();
}

void unlockForMilliseconds(int milliseconds) {
    /*
        Record unlock
    */
   lastUnlockAt = millis();
   unlockDurationMilliseconds = milliseconds;

    // Set solenoid to unlocked position
    setUnlocked();
}

int isUnlocked() {
    /*
          0 = No -- we're locked
        > 0 = Yes -- value represents how much longer we'll be unlocked
    */
   solenoidLoop(); // Ensure we're in the correct state
   int pinValue = digitalRead(unlockSolenoidPin); // Read pin state
   if (pinValue) {
       return (int)(millis() - (lastUnlockAt + unlockDurationMilliseconds));
   }
   return 0;
}


/*
    Serial API
*/
const size_t bufferSize = BUFFER_SIZE;
char incomingBuffer[BUFFER_SIZE];
char cmdBuffer[BUFFER_SIZE];
volatile int bufferPos = 0;
bool serialEchoEnabled = false;


const char SerialAPIPrefix = '$';

/*
    Parameterized Commands
*/
const char *SerialCommandDisplayMessagePrefix = "display:"; // Message follows
const char *SerialCommandDisplayMessageColoredPrefix = "display-color:"; // R, G, B, Message (w/o spaces)
const char *SerialCommandUnlockDurationPrefix = "unlock:"; // Duration in milliseconds


/*
    Generic ACK
*/
void printCmdResultOK() {
    Serial.println("<OK");
}

/*
    Commands that require input
*/
void cmdDisplayMessage(char *message) {
    // TODO
    Serial.print("Displaying ");
    Serial.println(message);
    printCmdResultOK();
}

void cmdDisplayMessageWithColor(char *message, int r, int g, int b) {
    // TODO
    Serial.print("Displaying ");
    Serial.println(message);
    printCmdResultOK();
}

void cmdRequestUnlockWithDuration(int durationMilliseconds) {
    unlockForMilliseconds(durationMilliseconds);
    printCmdResultOK();
}



/*
    Basic Commands
*/
const char *SerialCommandDeviceInfo = "info";
const char *SerialCommandDeviceSelfTest = "test";
const char *SerialCommandUnlock = "unlock";
const char *SerialCommandLock = "lock";
const char *SerialCommandUserList = "user-list";
const char *SerialCommandUserAdd = "user-add";
const char *SerialCommandUserAddCancel = "user-add-cancel";


/*
    Commands requiring a specific response
*/
void cmdGetDeviceInfo() {
    Serial.println("<Super Locker v0.0");
}

void cmdRequestUsers() {
    printCmdResultOK();
}


/*
    Commands only requiring an 'OK' response
*/
void cmdRequestSelfTest() {
    printCmdResultOK();
}


void cmdRequestUnlock() {
    unlockForMilliseconds(3000);
    printCmdResultOK();
}

void cmdRequestLock() {
    unlockDurationMilliseconds = 0;
    setLocked();
    printCmdResultOK();
}


void cmdBeginUserEnrollment() {
    printCmdResultOK();
    enrollFingerprint();
}

void cmdCancelUserEnrollment() {
    printCmdResultOK();
}


void serialInit() {
    bufferPos = 0;
    memset(&incomingBuffer, '\0', bufferSize);
    if(serialEchoEnabled) {
        Serial.print("> ");
    }
}


void processCommand() {   
    if (strcmp(incomingBuffer, "ATI") == 0) {
        Serial.println("Super Locker v0.0");
    }
    if (strcmp(incomingBuffer, "ECHO:ON") == 0) {
        serialEchoEnabled = true;
        Serial.println("Echo is on");
    }
    if (strcmp(incomingBuffer, "ECHO:OFF") == 0) {
        serialEchoEnabled = false;
        Serial.println("Echo is off");
    }
}

void processMachineCommand() {
    int parameterSeparatorIndex = get_index(incomingBuffer, ':');
    if(parameterSeparatorIndex > 1) {
        // Parameterized commands
        memset(&cmdBuffer, '\0', bufferSize);
        memcpy(&cmdBuffer[0], &incomingBuffer[1], parameterSeparatorIndex);
        if (strcmp(cmdBuffer, SerialCommandUnlockDurationPrefix) == 0) {
            char duration[10];
            memset(&duration, '\0', 10);
            memcpy(&duration[0], &incomingBuffer[parameterSeparatorIndex + 1], max(bufferPos, 10));
            int d = atoi(duration);
            cmdRequestUnlockWithDuration(d);
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandDisplayMessagePrefix) == 0) {
            char message[30];
            memset(&message, '\0', 30);
            memcpy(&message[0], &incomingBuffer[parameterSeparatorIndex + 1], max(bufferPos, 30));
            cmdDisplayMessage(message);
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandDisplayMessageColoredPrefix) == 0) {
            // Message might look like '$display-color:24,255,0,Hello, World!'
            char red[4];
            char green[4];
            char blue[4];
            char message[30];
            memset(&red, '\0', 4);
            memset(&green, '\0', 4);
            memset(&blue, '\0', 4);
            memset(&message, '\0', 30);

            int paramStart = parameterSeparatorIndex + 1;

            int commas[3] = {0, 0, 0};
            int commaIdx = 0;
            for(int i = paramStart; i < bufferPos; i++) {
                if(incomingBuffer[i] == ',' && commaIdx < 3) {
                    commas[commaIdx++] = i;
                }
            }

            memcpy(&red[0], &incomingBuffer[paramStart], commas[0] - paramStart);
            memcpy(&green[0], &incomingBuffer[commas[0] + 1], commas[1] - commas[0] - 1);
            memcpy(&blue[0], &incomingBuffer[commas[1] + 1], commas[2] - commas[1] -1 );

            memcpy(&message[0], &incomingBuffer[commas[2] + 1], max(bufferPos, 30));

            int redVal = atoi(red);
            int greenVal = atoi(green);
            int blueVal = atoi(blue);

            cmdDisplayMessageWithColor(message, redVal, greenVal, blueVal);
            return;
        }
    } else {
        // Basic commands
        memset(&cmdBuffer, '\0', bufferSize);
        memcpy(&cmdBuffer[0], &incomingBuffer[1], bufferPos - 1);
        if (strcmp(cmdBuffer, SerialCommandDeviceInfo) == 0) {
            cmdGetDeviceInfo();
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandDeviceSelfTest) == 0) {
            cmdRequestSelfTest();
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandUnlock) == 0) {
            cmdRequestUnlock();
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandLock) == 0) {
            cmdRequestLock();
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandUserList) == 0) {
            cmdRequestUsers();
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandUserAdd) == 0) {
            cmdBeginUserEnrollment();
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandUserAddCancel) == 0) {
            cmdCancelUserEnrollment();
            return;
        }
    }
}

//Returns the index of the first occurence of char c in char* string. If not found -1 is returned.
int get_index(char* string, char c) {
    char *e = strchr(string, c);
    if (e == NULL) {
        return -1;
    }
    return (int)(e - string);
}

void serviceSerial() {
    while (Serial.available()) {
        char data = Serial.read();
        if (data == 0x0A || data == 0x0D) {

            if (incomingBuffer[0] == SerialAPIPrefix) {
                processMachineCommand();
            } else {
                if (serialEchoEnabled) {
                    Serial.println();
                }
                Serial.println(incomingBuffer);
                processCommand();
            }

            serialInit();
        } else {
            if (serialEchoEnabled) {
                Serial.print(data);
            }
            if (bufferPos + 1 < bufferSize - 1) {
                incomingBuffer[bufferPos++] = data;
            }
        }
    }
}

void serviceFingerprintSensor() {
    if (flashErrorUntil < millis()) {
        fps.SetLED(true);
        if (fingerDetected()) {
            int id = getFingerprintUserID();
            if (id < 0) {
                Serial.println("!unrecognized-user");
                flashErrorUntil = millis() + 1000;
                return;
            }

            if (verifiedUserID == 0) {
                verifiedUserID = id;
                verifiedUserAt = millis();
                Serial.print("!authorized-user:");
                Serial.println(verifiedUserID, DEC);
                unlockForMilliseconds(1000);
            }

        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Super Locker");
    serialInit();
    setupSolenoid();
    #ifdef FPS_ENABLED
    fpsInit();
    #endif
}

void loop() {
    serviceSerial();

    #ifdef FPS_ENABLED
    serviceFingerprintSensor();
    fingerprintVerificationLoop();
    #endif

    solenoidLoop();
}