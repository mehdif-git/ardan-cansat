# ESP32_BMP280_Test

Ce projet permet de tester le module BMP 280 sur une carte ESP32. Le code principal lit les données de température et de pression du capteur et les affiche sur le moniteur série.

## Matériel requis

- Carte ESP32
- Module BMP 280
- Câbles de connexion

## Connexions

| BMP 280 Pin | ESP32 Pin |
|-------------|-----------|
| VCC         | 3.3V     |
| GND         | GND      |
| SDA         | GPIO 21  |
| SCL         | GPIO 22  |

## Installation des bibliothèques

Pour utiliser le module BMP 280, vous devez installer la bibliothèque Adafruit BMP280. Vous pouvez le faire via le gestionnaire de bibliothèques de l'IDE Arduino :

1. Ouvrez l'IDE Arduino.
2. Allez dans `Sketch` > `Include Library` > `Manage Libraries...`
3. Recherchez "Adafruit BMP280" et installez la bibliothèque.

## Exécution du code

1. Téléversez le code contenu dans `ESP32_BMP280_Test.ino` sur votre carte ESP32.
2. Ouvrez le moniteur série (à 115200 bauds) pour voir les données de température et de pression affichées.

## Remarques

Assurez-vous que votre module BMP 280 est correctement connecté à l'ESP32 avant de téléverser le code. Si vous rencontrez des problèmes, vérifiez les connexions et assurez-vous que la bibliothèque est correctement installée.