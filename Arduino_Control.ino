#include <Wire.h>

#define VOLTAGE_PIN A0
#define CURRENT_PIN A1
#define VOLTAGE_ZC_PIN 2
#define CURRENT_ZC_PIN 3

const int COMPONENT_PINS[3] = {6, 4, 5};
volatile unsigned long voltageTime = 0, currentTime = 0, timeDifference = 0;
float frequency = 50.0;

float totalEnergy = 0, dailyEnergy = 0, currentPower = 0;
unsigned long previousTime = 0;
const float timeInterval = 1.0;
bool componentStates[3] = {true, true, true}; // Start ON

const float threshold1 = 0.1;
const float threshold2 = 0.3;

unsigned long lastDailyReset = 0, lastMonthlyReset = 0;
const unsigned long dayDuration = 20000;
const unsigned long monthDuration = 40000;
unsigned long lastDataTransmission = 0;
const unsigned long dataTransmissionInterval = 1000;

void voltageISR() { voltageTime = micros(); }
void currentISR() {
  currentTime = micros();
  if (voltageTime > 0) {
    timeDifference = abs(currentTime - voltageTime);
  }
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_PIN, INPUT);
  pinMode(VOLTAGE_ZC_PIN, INPUT_PULLUP);
  pinMode(CURRENT_ZC_PIN, INPUT_PULLUP);

  for (int i = 0; i < 3; i++) {
    pinMode(COMPONENT_PINS[i], OUTPUT);
    digitalWrite(COMPONENT_PINS[i], LOW); // ON
    componentStates[i] = true;
  }

  attachInterrupt(digitalPinToInterrupt(VOLTAGE_ZC_PIN), voltageISR, RISING);
  attachInterrupt(digitalPinToInterrupt(CURRENT_ZC_PIN), currentISR, RISING);

  lastDailyReset = millis();
  lastMonthlyReset = millis();
  lastDataTransmission = millis();

  // Send initial status
  Serial.print("DATA:");
  Serial.print(currentPower);
  Serial.print(",");
  Serial.print(dailyEnergy);
  Serial.print(",");
  Serial.print(totalEnergy);
  for (int i = 0; i < 3; i++) {
    Serial.print(",");
    Serial.print(componentStates[i] ? "ON" : "OFF");
  }
  Serial.print(",");
  Serial.println(0); // Initial threshold level
}

float calculateInstantaneousPower() {
  if (timeDifference > 0) {
    float phaseAngle = (2.0 * PI * frequency * timeDifference) / 1000000.0;
    float powerFactor = cos(phaseAngle);
    float voltage = (analogRead(VOLTAGE_PIN) / 1023.0) * 230.0;
    float current = (analogRead(CURRENT_PIN) / 1023.0) * 0.67;
    float power = voltage * current * abs(powerFactor);
    return (power < 0.5) ? 0 : power; // Noise filter
  }
  return 0;
}

void processCommand(String cmd) {
  if (cmd.startsWith("TOGGLE:")) {
    int id = cmd.substring(7, 8).toInt() - 1;
    String state = cmd.substring(9);
    bool newState = (state == "ON");
    componentStates[id] = newState;
    digitalWrite(COMPONENT_PINS[id], newState ? LOW : HIGH);  // LOW = ON
  } else if (cmd == "TURNOFFALL") {
    for (int i = 0; i < 3; i++) {
      componentStates[i] = false;
      digitalWrite(COMPONENT_PINS[i], HIGH); // OFF
    }
  }
}
void sendUpdatedState() {
  // Send updated states to the serial monitor
  int thresholdVal = (dailyEnergy >= threshold2) ? 2 : (dailyEnergy >= threshold1) ? 1 : 0;
  Serial.print("DATA:");
  Serial.print(currentPower);
  Serial.print(",");
  Serial.print(dailyEnergy);
  Serial.print(",");
  Serial.print(totalEnergy);
  for (int i = 0; i < 3; i++) {
    Serial.print(",");
    Serial.print(componentStates[i] ? "ON" : "OFF");
  }
  Serial.print(",");
  Serial.println(thresholdVal);
}
void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousTime >= 1000) {
    currentPower = calculateInstantaneousPower();
    dailyEnergy += currentPower / 3600.0;
    previousTime = currentMillis;
  }

  if (currentMillis - lastDataTransmission >= dataTransmissionInterval) {
    sendUpdatedState();
    lastDataTransmission = currentMillis;
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length()) processCommand(cmd);
  }

  if (currentMillis - lastDailyReset >= dayDuration) {
    totalEnergy += dailyEnergy;
    dailyEnergy = 0;
    lastDailyReset = currentMillis;
    Serial.println("RESET:DAILY");
  }

  if (currentMillis - lastMonthlyReset >= monthDuration) {
    totalEnergy = 0;
    lastMonthlyReset = currentMillis;
    Serial.println("RESET:MONTHLY");
  }
}