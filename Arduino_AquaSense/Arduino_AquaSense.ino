#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>

// Pin Definitions
#define TURBIDITY_PIN A1  // Analog pin for turbidity sensor
#define PH_PIN A0         // Analog pin for pH sensor
#define ONE_WIRE_BUS 2    // Digital pin for DS18B20 sensor
#define WATER_PUMP_PIN 8  // Digital pin to control the water pump (you can use any digital pin)

// Initialize OneWire and DallasTemperature objects
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Setup software serial for communication with NodeMCU (connected to RX/TX)
SoftwareSerial nodeMCU(10, 11);  // RX, TX pins for NodeMCU communication

// Calibration constants for pH sensor
float neutralVoltage = 2.5; // Voltage at pH 7.0 (adjust during calibration)
float sensitivity = 0.18;   // Voltage difference per pH unit (adjust during calibration)

void setup() {
  // Start Serial communication for debugging
  Serial.begin(9600);
  nodeMCU.begin(9600);  // Start communication with NodeMCU

  // Initialize DS18B20 sensor
  sensors.begin();

  // Setup pin modes
  pinMode(TURBIDITY_PIN, INPUT);
  pinMode(PH_PIN, INPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);  // Initialize the water pump pin

  // Initial log
  Serial.println("Sensor Initialization Complete");
}

void loop() {
  // Read and process sensor data
  float turbidityValue = readTurbidity();
  float phValue = readPH();
  float temperature = readTemperature();

  // Prepare data to send to NodeMCU (via software serial)
  sendDataToNodeMCU(turbidityValue, phValue, temperature);

  // Check if data from NodeMCU is received to control the water pump
  if (nodeMCU.available()) {
    String receivedData = nodeMCU.readStringUntil('\n');
    Serial.println("Received from NodeMCU: " + receivedData);

    // Check if the pump control command is received
    if (receivedData == "TURN_ON_PUMP") {
      digitalWrite(WATER_PUMP_PIN, HIGH);  // Turn on pump
      Serial.println("Water Pump ON");
    } else if (receivedData == "TURN_OFF_PUMP") {
      digitalWrite(WATER_PUMP_PIN, LOW);  // Turn off pump
      Serial.println("Water Pump OFF");
    }
  }

  // Wait before sending the next batch of data
  delay(5000); // Delay 5 seconds
}

// Function to read the turbidity sensor (analog)
float readTurbidity() {
  int turbidityRaw = analogRead(TURBIDITY_PIN);  // Read analog value (0-1023)
  float voltage = turbidityRaw * (5.0 / 1023.0); // Convert to voltage (5V system)
  
  // Example mapping: Adjust based on your sensor's datasheet/calibration
  float turbidityNTU = map(voltage * 1000, 0, 4500, 0, 1000); 
  Serial.print("Turbidity (NTU): ");
  Serial.println(turbidityNTU);
  return turbidityNTU;
}

// Function to read pH sensor
float readPH() {
  int phAnalogValue = analogRead(PH_PIN);  // Read the pH sensor value (0-1023)
  float phVoltage = phAnalogValue * (5.0 / 1023.0);  // Convert to voltage
  float phValue = 7 + ((phVoltage - neutralVoltage) / sensitivity);  // Calculate pH
  Serial.print("pH Level: ");
  Serial.println(phValue);
  return phValue;
}

// Function to read temperature from DS18B20
float readTemperature() {
  sensors.requestTemperatures();  // Request temperature reading
  float tempC = sensors.getTempCByIndex(0);  // Get temperature in Celsius
  Serial.print("Temperature (C): ");
  Serial.println(tempC);
  return tempC;
}

// Function to send sensor data to NodeMCU via Software Serial
void sendDataToNodeMCU(float turbidity, float ph, float temperature) {
  String data = "Turbidity:" + String(turbidity, 1) + 
                ", pH:" + String(ph, 2) + 
                ", Temp:" + String(temperature, 1) + "C";

  nodeMCU.println(data);  // Send data to NodeMCU
  Serial.println("Sent to NodeMCU: " + data);
}
