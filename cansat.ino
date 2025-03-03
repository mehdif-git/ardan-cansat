#include <RHDatagram.h>
#include <RHGenericDriver.h>
#include <RHMesh.h>
#include <RHReliableDatagram.h>
#include <RHRouter.h>
#include <RHUartDriver.h>
#include <RH_RF95.h>
#include <RadioHead.h>

#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"
#include "SD_MMC.h"
#include "time.h"
#include <Wire.h>
#define sensor_t adafruit_sensor_t
#include <Adafruit_BMP280.h>
#undef sensor_t
// Configuration des broches ESP32-CAM AI-Thinker
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

// Configuration I2C pour BMP280
#define I2C_SDA      26    // Ces broches peuvent être modifiées selon votre câblage
#define I2C_SCL      27    // Assurez-vous qu'elles ne sont pas utilisées par la caméra

// Configuration UART pour RF95/96
#define COMSerial Serial1
#define ShowSerial Serial

RH_RF95<HardwareSerial> rf95(COMSerial);

// Intervalle entre les mesures (en millisecondes)
const unsigned long INTERVAL = 10000;  // 10 secondes
unsigned long lastCaptureTime = 0;

// Objets
Adafruit_BMP280 bmp;
TwoWire I2CBMP = TwoWire(0);

bool initCamera() {
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Configuration pour la carte SD
  if(psramFound()) {
    config.frame_size = FRAMESIZE_VGA;    // VGA: 640x480
    config.jpeg_quality = 10;             // 0-63, qualité plus basse = meilleur
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Initialiser la caméra
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("L'initialisation de la caméra a échoué avec l'erreur 0x%x", err);
    return false;
  }
  return true;
}

bool initSDCard() {
  if (!SD_MMC.begin()) {
    Serial.println("Erreur d'initialisation de la carte SD");
    return false;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("Pas de carte SD insérée");
    return false;
  }
  return true;
}

bool initBMP280() {
  I2CBMP.begin(I2C_SDA, I2C_SCL);
  if (!bmp.begin(0x76)) {
    Serial.println("Capteur BMP280 non détecté");
    return false;
  }
  
  // Configuration par défaut pour le BMP280
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     // Mode de fonctionnement
                  Adafruit_BMP280::SAMPLING_X2,      // Suréchantillonnage température
                  Adafruit_BMP280::SAMPLING_X16,     // Suréchantillonnage pression
                  Adafruit_BMP280::FILTER_X16,       // Filtrage
                  Adafruit_BMP280::STANDBY_MS_500);  // Temps d'attente

  return true;
}


bool setupLoRa() {
    ShowSerial.begin(115200);
    ShowSerial.println("RF95 client test.");

    if (!rf95.init()) {
        ShowSerial.println("init failed");
        return false;
    }

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
    // you can set transmitter powers from 5 to 23 dBm:
    //rf95.setTxPower(13, false);
    rf95.setTxPower(23,false);
    rf95.setFrequency(868.0);
    return true;
}

void captureAndSavePhoto() {
  // Capture d'une image
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Échec de la capture");
    return;
  }
  
  // Préparer le nom du fichier basé sur le timestamp
  char filename[32];
  sprintf(filename, "/photo_%lu.jpg", millis());
  
  // Enregistrement sur la carte SD
  fs::FS &fs = SD_MMC;
  File file = fs.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Échec de l'ouverture du fichier en écriture");
  } else {
    file.write(fb->buf, fb->len);
    Serial.printf("Photo enregistrée: %s\n", filename);
    file.close();
  }
  
  // Libération de la mémoire
  esp_camera_fb_return(fb);
}

void readAndSaveSensorData() {
  // Lecture des données du BMP280
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F; // hPa
  float altitude = bmp.readAltitude(1013.25); // Altitude en mètres (1013.25 = pression au niveau de la mer)
  
  // Préparer les données à enregistrer
  char data[100];
  sprintf(data, "Timestamp: %lu, Temperature: %.2f°C, Pressure: %.2f hPa, Altitude: %.2f m", 
          millis(), temperature, pressure, altitude);
  
  // Enregistrement sur la carte SD
  fs::FS &fs = SD_MMC;
  File file = fs.open("/sensor_data.csv", FILE_APPEND);
  if (!file) {
    Serial.println("Échec de l'ouverture du fichier en écriture");
    // Créer le fichier s'il n'existe pas
    file = fs.open("/sensor_data.csv", FILE_WRITE);
    if (!file) {
      Serial.println("Impossible de créer le fichier");
      return;
    }
    // Ajouter les en-têtes du CSV
    file.println("Timestamp,Temperature,Pressure,Altitude");
  }
  
  // Écrire les données au format CSV
  char csvData[100];
  sprintf(csvData, "%lu,%.2f,%.2f,%.2f", millis(), temperature, pressure, altitude);
  file.println(csvData);
  file.close();
  
  // Envoyer les données via LoRa
  rf95.send((uint8_t*)data, sizeof(data));
  rf95.waitPacketSent();

  Serial.println("Données de capteur enregistrées et transmises via LoRa");
  Serial.printf("Temperature: %.2f°C, Pressure: %.2f hPa, Altitude: %.2f m\n", 
                temperature, pressure, altitude);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Démarrage...");
  
  // Initialiser le système
  bool cameraInitialized = initCamera();
  bool sdCardInitialized = initSDCard();
  bool bmpInitialized = initBMP280();
  bool loraInitialized = setupLoRa();
  
  if (cameraInitialized && sdCardInitialized && bmpInitialized && loraInitialized) {
    Serial.println("Système initialisé avec succès");
  } else {
    Serial.println("Erreur d'initialisation du système");
    if (!cameraInitialized) Serial.println("Caméra: échec");
    if (!sdCardInitialized) Serial.println("Carte SD: échec");
    if (!bmpInitialized) Serial.println("BMP280: échec");
    if (!loraInitialized) Serial.println("LoRa: échec");
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Vérifier si c'est le moment de capturer de nouvelles données
  if (currentMillis - lastCaptureTime >= INTERVAL) {
    lastCaptureTime = currentMillis;
    
    // Capture et sauvegarde d'une photo
    captureAndSavePhoto();
    
    // Lecture, enregistrement et transmission des données du capteur
    readAndSaveSensorData();
  }
  
  // Attente courte pour éviter de surcharger le processeur
  delay(100);
}