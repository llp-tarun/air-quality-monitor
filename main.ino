#define BLYNK_TEMPLATE_ID "TMPL3oG35C-KU"
#define BLYNK_TEMPLATE_NAME "air quality monitor"
#define BLYNK_AUTH_TOKEN "5BcNq0nSvq9OjD433Ext5tfwEoQYB2Lu"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

const int MQ135pin = 34;
const int SMOKE_THRESHOLD = 10;
const float RL_VALUE = 10000.0; // 10kΩ load resistor - ADJUST TO YOUR VALUE

char ssid[] = "aqm";
char pass[] = "aqm1";

bool isRunning = false;
BlynkTimer timer;

void sendSensorData() {
  if (isRunning) {
    int rawValue = analogRead(MQ135pin);
    
    // ESP32 12-bit ADC (0-4095), 3.3V reference
    float voltage = (rawValue / 4095.0) * 3.3;
    
    // Corrected resistance calculation with RL value
    if (voltage < 3.3) {
      float resistance = (RL_VALUE * voltage) / (3.3 - voltage);
      
      // MQ135 PPM calculation (simplified curve fitting)
      float ppm = 1.0 / (0.4095 * pow((resistance / 146.9), -2.247));
      
      float co = ppm / 2.2;
      float methane = ppm / 2.7;
      float ammonia = ppm / 3.6;

      // Send to Blynk virtual pins
      Blynk.virtualWrite(V1, ppm);
      Blynk.virtualWrite(V5, co);
      Blynk.virtualWrite(V6, methane);
      Blynk.virtualWrite(V7, ammonia);

      // Multi-level alert system
      String status = "Safe";
      if (co > 50) status = "DANGER!";
      else if (co > 25) status = "WARNING";
      else if (co > SMOKE_THRESHOLD) status = "Caution";
      
      Blynk.virtualWrite(V10, status);
      
      // Serial debug output
      Serial.println("PPM:" + String(ppm) + " CO:" + String(co) + " Status:" + status);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(MQ135pin, INPUT);
  
  Serial.println("Starting MQ135 Air Quality Monitor...");
  Serial.println("Sensor warmup (60s)...");
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  // 60 second sensor warmup period
  delay(60000);
  
  Serial.println("Warmup complete. System ready!");
  
  // Timer runs every 2 seconds (better for sensor stability)
  timer.setInterval(2000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
  
  // WiFi reconnection logic
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.begin(ssid, pass);
    delay(5000);
  }
}

// FIXED: Proper BLYNK_WRITE syntax
BLYNK_WRITE(V4) {
  isRunning = param.asInt();
  Serial.println("Monitoring: " + String(isRunning ? "ON" : "OFF"));
  
  if (!isRunning) {
    Blynk.virtualWrite(V10, "Safe");
    Serial.println("Monitoring stopped - reset to Safe");
  }
}
