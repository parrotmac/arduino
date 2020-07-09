
#define BUFFER_SIZE 40

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
    Serial.println("OK");
}

/*
    Commands that require input
*/
void cmdDisplayMessage(char *message) {
    // TODO
    printCmdResultOK();
}

void cmdDisplayMessageWithColor(char *message, int r, int g, int b) {
    // TODO
    printCmdResultOK();
}

void cmdRequestUnlockWithDuration(int durationMilliseconds) {
    // TODO
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
    Serial.println("Super Locker v0.0");
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
    printCmdResultOK();
}

void cmdRequestLock() {
    printCmdResultOK();
}


void cmdBeginUserEnrollment() {
    printCmdResultOK();
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

void setup() {
    Serial.begin(115200);


    Serial.println("Welcome!");
    serialInit();
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
            // TODO: Call unlock with milliseconds
            return;
        }
        if (strcmp(cmdBuffer, SerialCommandDisplayMessagePrefix) == 0) {
            char message[30];
            memset(&message, '\0', 30);
            memcpy(&message[0], &incomingBuffer[parameterSeparatorIndex + 1], max(bufferPos, 30));
            Serial.print("Displaying ");
            Serial.println(message);
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

            Serial.print("Displaying ");
            Serial.println(message);
            Serial.println(redVal, DEC);
            Serial.println(greenVal, DEC);
            Serial.println(blueVal, DEC);
            Serial.println("------------");
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

void loop() {
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