#include "DHT.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
///////////////////////////
#define RED_PIN 18
#define GREEN_PIN 19
// Wifi credentials
//const char* ssid="Wokwi-GUEST"

// 1. WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// 2. ThingSpeak Write API Key
String apiKey = "091M67876OKG0SDT";

/////////////////////////////
// DHT Sensor configuration
#define DHTPIN 5
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
// Initialize LCD (Address: 0x27, 16 Columns, 2 Rows)
// Note: If 0x27 doesn't display anything on physical hardware, try 0x3F.
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32...");
//initializs pins
pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  // Initialize DHT sensor
  dht.begin();

  // Initialize LCD
  Wire.begin(21, 22); // SDA = GPIO 21, SCL = GPIO 22
  lcd.init();
  lcd.backlight();

  // Display initial status
  lcd.setCursor(0, 0);
  lcd.print("GROUP 3: PROJECT");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  delay(2000);
}

void loop() {
  // Read sensor values
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if readings failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("ERROR: DHT22 sensor not responding!");
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error!   ");
    delay(2000);
    return;
  }
///////// without conditions
 // Turn RED ON, GREEN OFF
 // Inside your loop() function:
if (temperature > 50.0) {
 /// Serial.println("hi");
  // lcd.print("hi");
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, LOW);
  delay(500);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  delay(500);
} 
else if (temperature < 10.0) {
 /// Serial.println("Low");
 digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, LOW);
  delay(2000);

   digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  delay(2000);
  // lcd.print("Low");
} 
else {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  delay(2000);
  // lcd.print("normal");
}
/////Connect to WiFi
// Connect to WiFi
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password, 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi Connected...");

  Serial.println("IP Address:");
  Serial.println(WiFi.localIP());
////////
  // Print to Serial Monitor
  Serial.println("======================");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
//Group 3 projects
lcd.setCursor(0, 0);
//lcd.print("GROUP 3: IOT DEVICE TEMP SENSOR");
  // Display Temperature on Line 1
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print((char)223); // Degree symbol
  lcd.print("C   ");

  // Display Humidity on Line 2
  lcd.setCursor(0, 2);
  lcd.print("Humidity: ");
  lcd.print(humidity, 1);
  lcd.print("%   ");

  // DHT22 sampling rate is ~0.5 Hz (reads every 2 and 4 seconds)
  delay(4000);
  lcd.clear();
  delay(2000);

}