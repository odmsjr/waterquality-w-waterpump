#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Firebase setup
#define WIFI_SSID "TIPIANS"
#define WIFI_PASSWORD ""
#define FIREBASE_HOST "aquasense-49bc0-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "lC4IyUTAd1MxXmQMNysTLTfZlHtqRCEt7GTcv91T"

// Set water pump control pin (D1 in this case)
#define WATER_PUMP_PIN D1

FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

void setup() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);

  // Initialize water pump pin
  pinMode(WATER_PUMP_PIN, OUTPUT);
  digitalWrite(WATER_PUMP_PIN, LOW); // Initially set pump to OFF

  // Initialize water pump state in Firebase (set initial state to OFF)
  Firebase.setBool(firebaseData, "/pump_state", false);

  // Ensure pump state is fetched correctly
  if (Firebase.getBool(firebaseData, "/pump_state")) {
    bool pumpState = firebaseData.boolData();
    Serial.println("Initial pump state: " + String(pumpState));
    if (pumpState) {
      digitalWrite(WATER_PUMP_PIN, HIGH); // Turn on pump if initial state is true
      Serial.println("Pump turned ON");
    } else {
      digitalWrite(WATER_PUMP_PIN, LOW); // Keep pump OFF if initial state is false
      Serial.println("Pump turned OFF");
    }
  }
}

void loop() {
  // Check Firebase for pump control state
  if (Firebase.getBool(firebaseData, "/pump_state")) {
    bool pumpState = firebaseData.boolData();
    Serial.println("Pump state from Firebase: " + String(pumpState));
    if (pumpState) {
      digitalWrite(WATER_PUMP_PIN, HIGH); // Turn on pump
      Serial.println("Pump turned ON");
    } else {
      digitalWrite(WATER_PUMP_PIN, LOW); // Turn off pump
      Serial.println("Pump turned OFF");
    }
  }

  // Check if any data is available from serial input
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    Serial.println("Received: " + data);
    sendDataToFirebase(data);
  }

  delay(1000); // Loop delay to prevent continuous querying
}

void sendDataToFirebase(String data) {
  // Assuming data format: "Turbidity:10.5, pH:7.0, Temp:25.0C"
  int turbidityIndex = data.indexOf("Turbidity:");
  int pHIndex = data.indexOf("pH:");
  int tempIndex = data.indexOf("Temp:");

  // Extract values from the received data
  float turbidity = data.substring(turbidityIndex + 10, data.indexOf(",", turbidityIndex)).toFloat();
  float pH = data.substring(pHIndex + 3, data.indexOf(",", pHIndex)).toFloat();
  float temp = data.substring(tempIndex + 5, data.indexOf("C", tempIndex)).toFloat();

  // Only send data to Firebase if valid values are extracted (greater than 0)
  if (turbidity > 0.0 && pH > 0.0 && temp > 0.0) {
    // Prepare the Firebase JSON structure with the desired format
    String timestamp = getTimestamp(); // You can get the timestamp dynamically if needed

    FirebaseJson sensorDataJson;

    FirebaseJson turbidityJson;
    turbidityJson.set("timestamp", timestamp);
    turbidityJson.set("value", String(turbidity));

    FirebaseJson pHJson;
    pHJson.set("timestamp", timestamp);
    pHJson.set("value", String(pH));

    FirebaseJson temperatureJson;
    temperatureJson.set("timestamp", timestamp);
    temperatureJson.set("value", String(temp));

    // Add sensor data to the main JSON object
    sensorDataJson.set("turbidity", turbidityJson);
    sensorDataJson.set("ph_level", pHJson);
    sensorDataJson.set("temperature", temperatureJson);

    // Send data to Firebase
    Serial.println("\nSending data to Firebase...");
    if (Firebase.set(firebaseData, "/sensor_data", sensorDataJson)) {
      Serial.println("Sensor data sent to Firebase successfully.");
    } else {
      Serial.println("Failed to send sensor data to Firebase!");
      Serial.println("Error: " + firebaseData.errorReason());
    }
  }
}

// Placeholder function for timestamp (can be replaced with actual time fetching)
String getTimestamp() {
  return "2024-12-08 10:00:00"; // Example timestamp
}