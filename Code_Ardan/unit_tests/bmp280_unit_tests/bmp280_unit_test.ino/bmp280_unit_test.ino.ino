#include <BMP280.h>
#include "Wire.h"

#define I2C_SDA 14
#define I2C_SCL 15
BMP280 bmp = BMP280(0x76);

void initBMP280() {
  Wire.setPins(I2C_SDA, I2C_SCL);
  Wire.begin(); 
  bmp.begin();
  delay(500);
}

void deinitBMP280(){
  bmp.reset();
  Wire.end();
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== BMP280 Unit Test ===");
  Serial.println("Initializing BMP280...");
  
  initBMP280();
  delay(300); // Init takes a bit of time
  
  Serial.println("BMP280 initialized successfully!");
  Serial.println("Starting measurements...");
  Serial.println();
}

void loop() {
  static unsigned long lastMeasurement = 0;
  static int measurementCount = 0;
  
  unsigned long currentTime = millis();
  
  // Prendre une mesure toutes les 2 secondes
  if (currentTime - lastMeasurement >= 2000) {
    measurementCount++;
    
    Serial.print("Measurement #");
    Serial.print(measurementCount);
    Serial.print(" (Time: ");
    Serial.print(currentTime / 1000.0, 1);
    Serial.println("s)");
    Serial.println("----------------------------------------");
    
    float temperature = bmp.getTemperature();
    uint32_t pressure = bmp.getPressure();
    int16_t altitude = bmp.calAltitude(pressure, 1013.25);
    
    Serial.print("Temperature: ");
    Serial.print(temperature, 2);
    Serial.println(" °C");
    
    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" Pa");
    
    Serial.print("Altitude: ");
    Serial.print(altitude);
    Serial.println(" m");
    
    Serial.println();
    
    lastMeasurement = currentTime;
  }
}

