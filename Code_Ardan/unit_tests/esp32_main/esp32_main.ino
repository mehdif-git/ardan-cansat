#include <BMP280.h>
#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include "Wire.h"
#include <EEPROM.h>
#include <RH_RF95.h>

#define TRANSISTOR_PIN    12
#define I2C_SDA 14
#define I2C_SCL 15
#define CAMERA_MODEL_ESP_EYE
#define EEPROM_SIZE 2
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

int pictureNumber = 0;
BMP280 bmp = BMP280(0x76);
File dataFile;
RH_RF95<HardwareSerial> rf95(Serial);

void sendData(float temperature, uint32_t pressure, int16_t altitude){
  char buf[64];
  int len = sprintf(buf, "%f %u %hd ", temperature, pressure, altitude);
  rf95.send((uint8_t*)buf, len);
}

void takePic(){
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;
  String path = "/picture" + String(pictureNumber) +".jpg";
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_WRITE); 
  file.write(fb->buf, fb->len);
  EEPROM.write(0, pictureNumber);
  EEPROM.commit();
  file.close();
  esp_camera_fb_return(fb); 
}

void saveDataToSD(float temperature, float pressure, float altitude) {
  dataFile = SD_MMC.open("/bmp280_data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.print(millis()/1000.0);
    dataFile.print(",");
    dataFile.print(temperature);
    dataFile.print(",");
    dataFile.print(pressure);
    dataFile.print(",");
    dataFile.println(altitude);
  }
}

void createDataFileIfNotExist() {
  if (!SD_MMC.exists("/bmp280_data.csv")) {
    dataFile = SD_MMC.open("/bmp280_data.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println("Timestamp,Temperature (C),Pressure (hPa),Altitude (m)");
      dataFile.close();
    }
  }
}

void initCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 

  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 5;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 5;
    config.fb_count = 1;
  }
  esp_err_t err = esp_camera_init(&config);
}

void deinitCamera(){
  esp_err_t err = esp_camera_deinit();
}

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

void initSDCard() {
  SD_MMC.begin("/sdcard",true);
  createDataFileIfNotExist();
}

void deinitSDCard(){
  SD_MMC.end();
}

void initLORA(){
  rf95.init();
  rf95.setTxPower(23, false);
  rf95.setFrequency(868.0);
}

void deinitLORA(){
  rf95.setModeIdle();
  rf95.sleep();
}

void setup() {
  pinMode(TRANSISTOR_PIN,OUTPUT);
  digitalWrite(TRANSISTOR_PIN, HIGH);
  delay(1000);
  digitalWrite(TRANSISTOR_PIN, LOW);
}

void loop() {

  initBMP280();
  float temperature = bmp.getTemperature();
  uint32_t pressure = bmp.getPressure();
  int16_t altitude = bmp.calAltitude(pressure,1013.25);
  deinitBMP280();
    
  initSDCard();
  saveDataToSD(temperature, pressure, altitude);
    
  initCamera();
  takePic();
  deinitCamera();
  deinitSDCard();

  initLORA();
  sendData(temperature, pressure, altitude);
  rf95.waitPacketSent();

  delay(1500);
}

