// potentiometers
const int NUM_SLIDERS = 6;
const int analogInputs[NUM_SLIDERS] = {A0, A1, A2, A3, A4, A5};
int analogSliderValues[NUM_SLIDERS];

// buttons
const int NUM_BUTTONS = 6;
const int buttonInputs[NUM_BUTTONS] = {7, 6, 5, 4, 3, 2};
int buttonValues[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};
int lastButtonValues[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};
unsigned long lastButtonTime[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};

// buttons animations
int buttonAnimState[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};
unsigned long buttonAnimTime[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};
unsigned long buttonClickedTime[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};

// clicked
int isMuted = 0;
int isDeafened = 0;

// shift register 74HC595 pins
const int CLOCK_PIN = 8;
const int LATCH_PIN = 9;
const int DATA_PIN = 10;
byte shifterData = 0b00000000;
const unsigned int SHIFTER_OFFSET = 1;

// initialize variables
unsigned int pwm = 0; // %

// ****************** timers ******************

// main
unsigned long time_millis = 0;

// pwm
unsigned long pwmInterval = 1; // 1ms
unsigned long pwmTime = 0;

// light animation
unsigned long lightInterval = 100; // 100ms
unsigned long lightTime = 0;

// sliders values
unsigned long sliderInterval = 20; // 100ms
unsigned long sliderTime = 0;

// send sliders values
unsigned long sendInterval = 20; // 20ms
unsigned long sendTime = 0;

// ****************** end ******************

void setup() {
    // set pins mode
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);

    for (int i = 0; i < NUM_SLIDERS; i++) {
        pinMode(analogInputs[i], INPUT);
    }

    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttonInputs[i], INPUT_PULLUP);
    }

    // start serial communication
    Serial.begin(115200);
}

void loop() {
    time_millis = millis();

    // read sliders values
    if (time_millis - sliderTime > sliderInterval) {
        updateSliderValues();
        sliderTime = time_millis;
    }

    // read buttons values
    if (time_millis - sendTime > sendInterval) {
        sendSliderValues();
        sendTime = time_millis;
    }

    // shifter pwm
    if (time_millis - pwmTime > pwmInterval) {
        // how strong the light is
        if (pwm == 9) {
            shifterData = 0b11111111;
        } else {
            shifterData = 0b00000000;
        }

        // clicked buttons
        for (int i = 0; i < NUM_BUTTONS; i++) {
            if (buttonValues[i] == 0) {
                shifterData |= 1 << i + SHIFTER_OFFSET;

                buttonClickedTime[i] = time_millis;
            }
        }

        // active buttons animations
        // mute blink
        if (isMuted || analogSliderValues[4] == 0) {
            blinkButton(0);
        } else if (buttonAnimState[0] == 0) {
            buttonAnimState[0] = 1;
        }
        // deafen blink
        if (isDeafened) {
            blinkButton(1);
        } else if (buttonAnimState[1] == 0) {
            buttonAnimState[1] = 1;
        }

        // light a button after it was clicked
        for (int i = 0; i < NUM_BUTTONS; i++) {
            if (time_millis - buttonClickedTime[i] < 500) {
                shifterData |= 1 << i + SHIFTER_OFFSET;
            }
        }

        // send data to the shift register
        digitalWrite(LATCH_PIN, LOW);
        shiftOut(shifterData);
        digitalWrite(LATCH_PIN, HIGH);

        // pwm
        pwm++;
        if (pwm >= 10) {
            pwm = 0;
        }
        pwmTime = time_millis;
    }
}

/**
 * Blink the button
 * @param buttonIndex
 */
void blinkButton(int buttonIndex) {
    if (time_millis - buttonAnimTime[buttonIndex] > 500) {
        buttonAnimState[buttonIndex] = !buttonAnimState[buttonIndex];

        buttonAnimTime[buttonIndex] = time_millis;
    }

    if (buttonAnimState[buttonIndex] == 0) {
        shifterData |= 1 << buttonIndex + SHIFTER_OFFSET;
    }
}

/**
 * Send data to the shift register
 * @param data
 */
void shiftOut(byte data) {
    int i = 0;
    int pinState;

    digitalWrite(DATA_PIN, 0);
    digitalWrite(CLOCK_PIN, 0);

    for (i = 7; i >= 0; i--) {
        digitalWrite(CLOCK_PIN, 0);

        if (data & (1 << i)) {
            pinState = 1;
        } else {
            pinState = 0;
        }

        digitalWrite(DATA_PIN, pinState);
        digitalWrite(CLOCK_PIN, 1);
        digitalWrite(DATA_PIN, 0);
    }

    digitalWrite(CLOCK_PIN, 0);
}

/**
 * Read the values of the sliders
 */
void updateSliderValues() {
    for (int i = 0; i < NUM_SLIDERS; i++) {
        analogSliderValues[i] = analogRead(analogInputs[i]);
    }
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttonValues[i] = digitalRead(buttonInputs[i]);
    }

    // set button states
    // mute
    if (buttonValues[0] == 0 && lastButtonValues[0] != buttonValues[0] && time_millis - lastButtonTime[0] > 100) {
        isMuted = !isMuted;
        lastButtonTime[0] = time_millis;
    }

    // deafen
    if (buttonValues[1] == 0 && lastButtonValues[1] != buttonValues[1] && time_millis - lastButtonTime[1] > 100) {
        isDeafened = !isDeafened;
        lastButtonTime[1] = time_millis;
    }

    // save the last values
    for (int i = 0; i < NUM_BUTTONS; i++) {
        lastButtonValues[i] = buttonValues[i];
    }
}

/**
 * Send the values of the sliders to the serial port
 */
void sendSliderValues() {
    String builtString = String("");

    for (int i = 0; i < NUM_SLIDERS; i++) {
        builtString += "s";
        builtString += String((int) analogSliderValues[i]);

        if (i < NUM_SLIDERS - 1) {
            builtString += String("|");
        }
    }

    if (NUM_BUTTONS > 0) {
        builtString += String("|");
    }

    for (int i = 0; i < NUM_BUTTONS; i++) {
        builtString += "b";
        builtString += String((int) buttonValues[i]);

        if (i < NUM_BUTTONS - 1) {
            builtString += String("|");
        }
    }

    Serial.println(builtString);
}
