#include <SPI.h>
#include <MFRC522.h>
#include <DHT.h>

// Pin Definitions for Arduino Uno / Nano
#define RST_PIN     9     // RFID Reset Pin
#define SS_PIN      10    // RFID SDA / SS Pin
#define DHT_PIN     2     // DHT Sensor Data Pin
#define DHTTYPE     DHT11 // Change to DHT22 if using DHT22

#define RED_LED_PIN 7     // Critical Alert LED (> 37°C)
#define GRN_LED_PIN 6     // Normal Status LED
#define BUZZER_PIN  5     // Buzzer alarm output

// Initialize Hardware Objects
MFRC522 rfid(SS_PIN, RST_PIN);
DHT dht(DHT_PIN, DHTTYPE);

// Thermal Threshold
const float TEMP_THRESHOLD = 37.0; // °C

void setup() {
  // Start Serial Communication at 9600 baud
  Serial.begin(9600);
  while (!Serial);

  // Initialize SPI bus and RFID reader
  SPI.begin();
  rfid.PCD_Init();

  // Initialize DHT temperature/humidity sensor
  dht.begin();

  // Configure indicator pins
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GRN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Startup indicator blink
  digitalWrite(GRN_LED_PIN, HIGH);
  delay(400);
  digitalWrite(GRN_LED_PIN, LOW);
}

void loop() {
  // 1. Read real sensor data (NO faked values)
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius

  // Check if sensor reading failed
  if (isnan(humidity) || isnan(temperature)) {
    // Send JSON error format so the web dashboard doesn't break
    Serial.println("{\"temp\": 0.0, \"humidity\": 0.0, \"status\": \"WARNING: SENSOR ERROR\"}");
    delay(2000);
    return;
  }

  // 2. Evaluate threshold (> 37°C Critical Alert)
  bool isCritical = (temperature > TEMP_THRESHOLD);

  if (isCritical) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GRN_LED_PIN, LOW);
    tone(BUZZER_PIN, 1000); // Sound buzzer alarm
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GRN_LED_PIN, HIGH);
    noTone(BUZZER_PIN);     // Stop buzzer
  }

  // 3. Check if an RFID tag is physically scanned
  String rfidStatus = "";
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Card detected! We can capture its UID if needed
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  // 4. Output strictly formatted JSON for your HTML dashboard
  Serial.print("{");
  Serial.print("\"temp\": ");
  Serial.print(temperature, 1);
  Serial.print(", \"humidity\": ");
  Serial.print(humidity, 1);
  Serial.print(", \"status\": \"");
  
  if (isCritical) {
    Serial.print("CRITICAL ALERT: TEMP > 37C");
  } else {
    Serial.print("SYSTEM NORMAL - ALL VITALS SECURE");
  }
  
  Serial.println("\"}");

  // Delay between readings (1.5 seconds)
  delay(1500);
}