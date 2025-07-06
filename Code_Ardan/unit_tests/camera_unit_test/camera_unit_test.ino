#include "esp_camera.h"
#include "Wire.h"
#include <EEPROM.h>
#include "SD_MMC.h"

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

void createDataFileIfNotExist() {
  // Fonction pour créer un fichier de données si nécessaire
  // Cette fonction peut être étendue selon les besoins
  Serial.println("Checking/creating data file structure...");
}

void takePic(){
  Serial.println("Starting picture capture...");
  
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  
  if (!fb) {
    Serial.println("ERROR: Camera capture failed!");
    return;
  }
  
  Serial.print("Picture captured successfully! Size: ");
  Serial.print(fb->len);
  Serial.println(" bytes");
  
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;
  String path = "/picture" + String(pictureNumber) +".jpg";
  
  Serial.print("Saving picture as: ");
  Serial.println(path);
  
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_WRITE);
  
  if (!file) {
    Serial.println("ERROR: Failed to open file for writing!");
    esp_camera_fb_return(fb);
    return;
  }
  
  file.write(fb->buf, fb->len);
  EEPROM.write(0, pictureNumber);
  EEPROM.commit();
  file.close();
  esp_camera_fb_return(fb);
  
  Serial.print("Picture #");
  Serial.print(pictureNumber);
  Serial.println(" saved successfully to SD card!");
  Serial.println("----------------------------------------");
}


void initCamera(){
  Serial.println("Initializing camera...");
  
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
    Serial.println("PSRAM found - using high quality settings");
    config.frame_size = FRAMESIZE_SVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 5;
    config.fb_count = 1;
  } else {
    Serial.println("PSRAM not found - using standard settings");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 5;
    config.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  
  if (err != ESP_OK) {
    Serial.print("ERROR: Camera init failed with error 0x");
    Serial.println(err, HEX);
    return;
  }
  
  Serial.println("Camera initialized successfully!");
}

void deinitCamera(){
  Serial.println("Deinitializing camera...");
  esp_err_t err = esp_camera_deinit();
  if (err != ESP_OK) {
    Serial.print("ERROR: Camera deinit failed with error 0x");
    Serial.println(err, HEX);
  } else {
    Serial.println("Camera deinitialized successfully!");
  }
}

void initSDCard() {
  Serial.println("Initializing SD Card...");
  
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("ERROR: SD Card Mount Failed!");
    return;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("ERROR: No SD card attached!");
    return;
  }
  
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.print("SD Card Size: ");
  Serial.print(cardSize);
  Serial.println(" MB");
  
  createDataFileIfNotExist();
  Serial.println("SD Card initialized successfully!");
}

void deinitSDCard(){
  Serial.println("Deinitializing SD Card...");
  SD_MMC.end();
  Serial.println("SD Card deinitialized!");
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== ESP32 Camera Unit Test ===");
  Serial.println("Initializing components...");
  Serial.println();
  
  initSDCard();
  Serial.println();
  
  initCamera();
  Serial.println();
  
  Serial.println("Taking initial test picture...");
  takePic();
  Serial.println();
  
  Serial.println("Setup complete! Camera test will take pictures every 5 seconds.");
  Serial.println("=====================================");
}

void loop() {
  static unsigned long lastPicture = 0;
  static int totalPictures = 0;
  
  unsigned long currentTime = millis();
  
  // Prendre une photo toutes les 5 secondes
  if (currentTime - lastPicture >= 5000) {
    totalPictures++;
    
    Serial.print("Taking picture #");
    Serial.print(totalPictures);
    Serial.print(" (Runtime: ");
    Serial.print(currentTime / 1000.0, 1);
    Serial.println("s)");
    
    takePic();
    
    Serial.print("Total pictures taken: ");
    Serial.println(totalPictures);
    Serial.println();
    
    lastPicture = currentTime;
  }
  
  // Petit délai pour éviter la surcharge du processeur
  delay(100);
}

