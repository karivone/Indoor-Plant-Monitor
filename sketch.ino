#include <WiFi.h>
#include <DHT.h>
#include <ThingSpeak.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// --- Wokwi Virtual Wi-Fi Settings ---
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// --- ThingSpeak Configuration ---
unsigned long myChannelNumber = 3572114; 
const char* myWriteAPIKey     = "FSBABX35I3STB53W";

// --- Pin Assignments ---
#define DHTPIN        4
#define DHTTYPE       DHT22

#define PIN_GREEN_LED 19
#define PIN_RED_LED   18
#define PIN_BUZZER    21

// --- I2C LCD Pins ---
#define I2C_SDA       17
#define I2C_SCL       16

// --- Thresholds ---
const float TEMP_MIN = 18.0;
const float TEMP_MAX = 30.0;

// --- Timing Controls (Non-blocking) ---
const unsigned long SENSOR_INTERVAL = 2000;   // Read sensor every 2 seconds
const unsigned long CLOUD_INTERVAL  = 20000;  // Upload every 20 seconds

unsigned long lastSensorRead = 0;
unsigned long lastCloudSend  = 0;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;
LiquidCrystal_I2C lcd(0x27, 16, 2);

enum PlantStatus {
  HEALTHY,
  TOO_COLD,
  TOO_HOT,
  SENSOR_ERROR
};

PlantStatus currentStatus = HEALTHY;
float currentTemp = 0.0;

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  Serial.print("Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected to network.");
  } else {
    Serial.println("\n[WiFi] Connection timeout. Retrying next cycle.");
  }
}

void triggerAlarm(bool hot) {
  if (hot) {
    // Too Hot: 3 rapid beeps
    for (int i = 0; i < 3; i++) {
      digitalWrite(PIN_BUZZER, HIGH);
      delay(120);
      digitalWrite(PIN_BUZZER, LOW);
      delay(80);
    }
  } else {
    // Too Cold: 2 longer warning beeps
    for (int i = 0; i < 2; i++) {
      digitalWrite(PIN_BUZZER, HIGH);
      delay(250);
      digitalWrite(PIN_BUZZER, LOW);
      delay(150);
    }
  }
}

void updateLCD(float temp, PlantStatus status) {
  lcd.setCursor(0, 0);
  if (status == SENSOR_ERROR) {
    lcd.print("Temp: ERROR     ");
    lcd.setCursor(0, 1);
    lcd.print("Check Sensor!   ");
    return;
  }

  // Row 1: Temperature display
  char tempBuffer[17];
  snprintf(tempBuffer, sizeof(tempBuffer), "Temp: %.1f C    ", temp);
  lcd.print(tempBuffer);

  // Row 2: Status display
  lcd.setCursor(0, 1);
  if (status == HEALTHY) {
    lcd.print("Status: OPTIMAL ");
  } else if (status == TOO_HOT) {
    lcd.print("Status: TOO HOT!");
  } else if (status == TOO_COLD) {
    lcd.print("Status: TOO COLD");
  }
}

void evaluatePlantConditions(float temp) {
  if (isnan(temp)) {
    currentStatus = SENSOR_ERROR;
    Serial.println("[ERROR] Failed to read from DHT22!");
    updateLCD(0.0, currentStatus);
    return;
  }

  if (temp < TEMP_MIN) {
    currentStatus = TOO_COLD;
    digitalWrite(PIN_GREEN_LED, LOW);
    digitalWrite(PIN_RED_LED, HIGH);
    Serial.printf("Temperature: %.1f°C | Status: TOO COLD\n", temp);
    updateLCD(temp, currentStatus);
    triggerAlarm(false);
  } 
  else if (temp > TEMP_MAX) {
    currentStatus = TOO_HOT;
    digitalWrite(PIN_GREEN_LED, LOW);
    digitalWrite(PIN_RED_LED, HIGH);
    Serial.printf("Temperature: %.1f°C | Status: TOO HOT\n", temp);
    updateLCD(temp, currentStatus);
    triggerAlarm(true);
  } 
  else {
    currentStatus = HEALTHY;
    digitalWrite(PIN_GREEN_LED, HIGH);
    digitalWrite(PIN_RED_LED, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    Serial.printf("Temperature: %.1f°C | Status: HEALTHY\n", temp);
    updateLCD(temp, currentStatus);
  }
}

void sendTelemetry(float temp, PlantStatus status) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    return;
  }

  ThingSpeak.setField(1, temp);

  int statusCode = (status == HEALTHY) ? 1 : (status == TOO_HOT ? 2 : 3);
  ThingSpeak.setField(2, statusCode);

  String statusMsg = (status == HEALTHY) ? "Healthy" : (status == TOO_HOT ? "Too Hot" : "Too Cold");
  ThingSpeak.setStatus(statusMsg);

  int response = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (response == 200) {
    Serial.println("[Cloud] Data uploaded successfully to ThingSpeak.");
  } else {
    Serial.printf("[Cloud] Upload failed with HTTP code %d\n", response);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_GREEN_LED, OUTPUT);
  pinMode(PIN_RED_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Start I2C bus with defined SDA/SCL pins
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();

  // Boot Splash Screen
  lcd.setCursor(0, 0);
  lcd.print("GROUP 3: PROJECT");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Self-test LED indicator
  digitalWrite(PIN_GREEN_LED, HIGH);
  digitalWrite(PIN_RED_LED, HIGH);
  delay(1000);
  digitalWrite(PIN_GREEN_LED, LOW);
  digitalWrite(PIN_RED_LED, LOW);

  lcd.clear();

  dht.begin();
  connectWiFi();
  ThingSpeak.begin(client);

  Serial.println("--- System Initialized ---");
}

void loop() {
  unsigned long now = millis();

  // Task 1: Read DHT22 & Refresh Local Alerts + LCD every 2 seconds
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    currentTemp = dht.readTemperature();
    evaluatePlantConditions(currentTemp);
  }

  // Task 2: Push Telemetry to ThingSpeak every 20 seconds
  if (now - lastCloudSend >= CLOUD_INTERVAL) {
    lastCloudSend = now;
    if (!isnan(currentTemp)) {
      sendTelemetry(currentTemp, currentStatus);
    }
  }
}