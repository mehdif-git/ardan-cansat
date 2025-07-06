/*
 * Ardan CanSat Main Code
 */

 #include <BMP280.h>
 #include "esp_camera.h"
 #include "FS.h"
 #include "SD_MMC.h"
 #include "Wire.h"
 #include <EEPROM.h>
 #include <RH_RF95.h>
 #include "esp_sleep.h"
 
 // === CONFIGURATION DES BROCHES ===
 #define I2C_SDA           14
 #define I2C_SCL           15
 #define TRANSISTOR_PIN    12
 #define EEPROM_SIZE       2
 
 // Configuration caméra ESP-EYE
 #define CAMERA_MODEL_ESP_EYE
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
 
 // === PARAMÈTRES DE DÉTECTION ===
 const int BUFFER_SIZE = 5;              // Taille du buffer de lissage
 const float FALL_THRESHOLD = -0.5;      // Seuil de vitesse pour détecter la chute (m/s)
 const float GROUND_THRESHOLD = 0.05;    // Seuil de vitesse pour détecter l'arrêt (m/s)
 const float LAUNCH_THRESHOLD = 2.0;     // Seuil de vitesse pour détecter le lancement (m/s)
 const float DT = 0.5;                   // Pas de temps en secondes
 const float MIN_FALL_DURATION = 10;    // Durée minimale de chute (secondes)
 const float MIN_LAUNCH_DURATION = 2;   // Durée minimale de lancement (secondes)
 const float STATIONARY_TIME_REQ = 10;  // Temps requis pour confirmer l'arrêt (secondes)
 
 // === VARIABLES GLOBALES ===
 int pictureNumber = 0;
 BMP280 bmp = BMP280(0x76);
 File dataFile;
 RH_RF95<HardwareSerial> rf95(Serial);
 
 // Variables de détection de mission
 float altitudeBuffer[BUFFER_SIZE];
 int bufferIndex = 0;
 float previousSmoothed = 0.0;
 float launchTime = 0.0;
 bool bufferFilled = false;
 float fallTime = 0.0;
 float stationaryTime = 0.0;
 bool launchDetected = false;
 bool fallDetected = false;
 bool onGround = false;
 
 // === FONCTIONS D'INITIALISATION ===
 
 void initCamera() {
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
   
   // Configuration selon la PSRAM disponible
   if (psramFound()) {
     config.frame_size = FRAMESIZE_SVGA;
     config.jpeg_quality = 5;
     config.fb_count = 1;
   } else {
     config.frame_size = FRAMESIZE_VGA; // Réduit pour économiser la mémoire
     config.jpeg_quality = 8;
     config.fb_count = 1;
   }
   
   esp_camera_init(&config);
 }
 
 void initBMP280() {
   Wire.setPins(I2C_SDA, I2C_SCL);
   Wire.begin();
   bmp.begin();
   delay(200);
 }
 
 void initSDCard() {
   SD_MMC.begin("/sdcard",true);
   createDataFileIfNotExist();
 }
 
 void initLORA() {
   rf95.init();
   rf95.setTxPower(23, false);
   rf95.setFrequency(868.125);
 }
 
 // === FONCTIONS DE DÉSINITIALISATION ===
 
 void deinitCamera() {
   esp_camera_deinit();
 }
 
 void deinitBMP280() {
   bmp.reset();
   Wire.end();
 }
 
 void deinitSDCard() {
   dataFile.close();
   SD_MMC.end();
 }
 
 void deinitLORA() {
   rf95.setModeIdle();
   rf95.sleep();
 }
 
 // === FONCTIONS UTILITAIRES ===
 
 void createDataFileIfNotExist() {
   if (!SD_MMC.exists("/bmp280_data.csv")) {
     dataFile = SD_MMC.open("/bmp280_data.csv", FILE_WRITE);
     if (dataFile) {
       dataFile.println("Timestamp,Temperature (C),Pressure (hPa),Altitude (m)");
       dataFile.close();
     }
   }
 }
 
 void saveDataToSD(float temperature, float pressure, float altitude) {
   dataFile = SD_MMC.open("/bmp280_data.csv", FILE_APPEND);
   if (dataFile) {
     dataFile.print(millis() / 1000.0);
     dataFile.print(",");
     dataFile.print(temperature);
     dataFile.print(",");
     dataFile.print(pressure);
     dataFile.print(",");
     dataFile.println(altitude);
     dataFile.close(); // Fermer immédiatement pour économiser l'énergie
   }
 }
 
 void takePicture() {
   camera_fb_t *fb = esp_camera_fb_get();
   if (!fb) return;
   
   EEPROM.begin(EEPROM_SIZE);
   pictureNumber = EEPROM.read(0) + 1;
   
   String path = "/picture" + String(pictureNumber) + ".jpg";
   File file = SD_MMC.open(path.c_str(), FILE_WRITE);
   if (file) {
     file.write(fb->buf, fb->len);
     file.close();
     
     EEPROM.write(0, pictureNumber);
     EEPROM.commit();
   }
   
   esp_camera_fb_return(fb);
 }
 
 void sendData(float temperature, uint32_t pressure, int16_t altitude) {
   char buffer[64];
   int len = sprintf(buffer,"%f %u %hd ", temperature, pressure, altitude);
   rf95.send((uint8_t*)buffer, len);
   rf95.waitPacketSent();
 }
 
 // === FONCTIONS DE DÉTECTION ===
 
 float getSmoothedAltitude(float rawAltitude) {
   // Mise à jour du buffer circulaire
   altitudeBuffer[bufferIndex] = rawAltitude;
   bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
   if (bufferIndex == 0) bufferFilled = true;
   
   // Calcul de la moyenne lissée
   float smoothedAltitude = 0.0;
   int count = bufferFilled ? BUFFER_SIZE : bufferIndex;
   for (int i = 0; i < count; i++) {
     smoothedAltitude += altitudeBuffer[i];
   }
   return smoothedAltitude / count;
 }
 
 void detectLaunch() {
  float rawAltitude = bmp.calAltitude(bmp.getPressure(), 1013.25);
  float smoothedAltitude = getSmoothedAltitude(rawAltitude);
  
  // Calcul de la vitesse (dérivée première)
  float velocity = (smoothedAltitude - previousSmoothed) / DT;
  previousSmoothed = smoothedAltitude;
  
  // Détection de lancement (vitesse ascendante soutenue)
  if (velocity > LAUNCH_THRESHOLD) {
    launchTime += DT;
    if (launchTime >= MIN_LAUNCH_DURATION) {
      launchDetected = true;
    }
  } else {
    launchTime = 0;
  }
}

 void detectFall() {
   float rawAltitude = bmp.calAltitude(bmp.getPressure(), 1013.25);
   float smoothedAltitude = getSmoothedAltitude(rawAltitude);
   
   // Calcul de la vitesse (dérivée première)
   float velocity = (smoothedAltitude - previousSmoothed) / DT;
   previousSmoothed = smoothedAltitude;
   
   // Détection de chute
   if (velocity < FALL_THRESHOLD) {
     fallTime += DT;
     if (fallTime >= MIN_FALL_DURATION) {
       fallDetected = true;
     }
   } else {
     fallTime = 0;
   }
 }
 
 void detectGroundContact() {
   initBMP280();
   float rawAltitude = bmp.calAltitude(bmp.getPressure(), 1013.25);
   deinitBMP280();
   float smoothedAltitude = getSmoothedAltitude(rawAltitude);
   
   // Calcul de la vitesse
   float velocity = (smoothedAltitude - previousSmoothed) / DT;
   previousSmoothed = smoothedAltitude;
   
   // Détection d'arrêt au sol
   if (abs(velocity) < GROUND_THRESHOLD) {
     stationaryTime += DT;
     if (stationaryTime >= STATIONARY_TIME_REQ) {
       onGround = true;
     }
   } else {
     stationaryTime = 0;
   }
 }
 
 // === MISSION PRINCIPALE ===
 
 void executeMainMission() {
   // Lecture des données du capteur
   initBMP280();
   float temperature = bmp.getTemperature();
   uint32_t pressure = bmp.getPressure();
   int16_t altitude = bmp.calAltitude(pressure, 1013.25);
   deinitBMP280();

   // Sauvegarde des données
   initSDCard();
   saveDataToSD(temperature, pressure, altitude);
   
   // Prise de photo
   initCamera();
   takePicture();
   deinitCamera();
   deinitSDCard();

   // Transmission LoRa
   sendData(temperature, pressure, altitude);
 }
 
 void enterDeepSleepWithTransistor() {
   // Désactivation de tous les périphériques
   deinitLORA();

   // Configuration du transistor en HIGH avant le sommeil
   digitalWrite(TRANSISTOR_PIN, HIGH);
   
   // Configuration pour maintenir l'état de la broche pendant le sommeil
   gpio_hold_en((gpio_num_t)TRANSISTOR_PIN);
   gpio_deep_sleep_hold_en();
   
   // Entrée en sommeil profond (réveil uniquement par reset)
   esp_deep_sleep_start();
 }
 
 // === PROGRAMME PRINCIPAL ===
 
 void setup() {
  // Making sure pin 12 is not latched on (for restarts only)
  pinMode(TRANSISTOR_PIN, OUTPUT);
  gpio_hold_dis((gpio_num_t)TRANSISTOR_PIN);
  gpio_deep_sleep_hold_dis();
  digitalWrite(TRANSISTOR_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSISTOR_PIN, LOW);
  initBMP280();
 }
 
 void loop() {
   while(!launchDetected){
    detectLaunch();
   }
   // Phase 1: Détection de chute
   while (!fallDetected) {
    detectFall();
   }
   deinitBMP280();
   // Phase 2: Mission principale (acquisition et transmission)
   initLORA();
   delay(200);
   uint8_t startMessage[] = "Started!\n"; // Messages start with 'm'
   rf95.send(startMessage, sizeof(startMessage));
   rf95.waitPacketSent();

   while (!onGround) {
     executeMainMission();
     detectGroundContact();
   }
   uint8_t endMessage[] = "Landed!\n"; 
   rf95.send(endMessage, sizeof(endMessage));
   rf95.waitPacketSent();
   
   // Phase 3: Mode récupération avec optimisation énergétique
   enterDeepSleepWithTransistor();
 }
