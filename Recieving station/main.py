"""
# SCALAR Va Charge Utile
# Programme principal d'AéroSat

"""

# Importations pour la carte SD
import os
from machine import Pin, SPI
import sdcard

# Importation pour le LoRa Wan
from sx1262 import SX1262

# Importations pour le buzzer
from buzzer import *

# Bibliothèques BMP280 (Capteur Pression et température)

#Définition de la variable du buzzer
BUZZER_ENABLE = True

# Buzzer de début d'initialisation
SetBuzzer(BUZZER_ENABLE, freq=800, tps=1)
time.sleep(1)
SetBuzzer(False)


"""boucle"""

if __name__ == '__main__':



    # Configuration de la carte SD
    SD_CS_PIN = 5
    SD_SCK_PIN = 6
    SD_MOSI_PIN = 7
    SD_MISO_PIN = 4
    spi = SPI(0, baudrate=100000, sck=Pin(SD_SCK_PIN), mosi=Pin(SD_MOSI_PIN), miso=Pin(SD_MISO_PIN))

    sd = sdcard.SDCard(spi, Pin(SD_CS_PIN))
    vfs = os.VfsFat(sd)
    os.mount(vfs, '/sd')

    # Configuration du LoRa Wan
    sx = SX1262(spi_bus=1, clk=10, mosi=11, miso=12, cs=3, irq=20, rst=15, gpio=2)

    sx.begin(freq=434.0, bw=125.0, sf=9, cr=7, syncWord=0x12,
            power=22, currentLimit=60.0, preambleLength=8,
            implicit=False, implicitLen=0xFF,
            crcOn=True, txIq=False, rxIq=False,
            tcxoVoltage=1.6, useRegulatorLDO=False, blocking=True)

    # Etat du vol
    landed = 0 # 0 tant que le Cansat n'a pas atterri
    launched = 0 # 0 tant que la fusée n'a pas décollé

    # Récupération de l'altitude sur la rampe de lancement
    launch_altitude = 0
    

    while launch_altitude == 0 :

        if gnss_l76b.uart_any():
                    sentence = parser.update(chr(gnss_l76b.uart_receive_byte()[0]))
                
                    if sentence:
                    
                        launch_altitude = parser.altitude



    # Les données du GPS et du PMS sont enregistrées sur la carte SD, dans le fichier data.csv.
    file = open('/sd/data.csv', 'a')
    file.write("execution_time, WGS84 _latitude_parser, WGS84_latitude, WGS84_longitude_parser, WGS84_longitude, UTC_timestamp, Altitude, PM1.0, PM2.5, PM10, PM1.0, PM2.5, PM10, >0.3um in 0.1L air, >0.5um in 0.1L air, >1.0um in 0.1L air, >2.5um in 0.1L air, >5.0um in 0.1L air, >10um in 0.1L air\n")    


    # Buzzer de fin d'initialisation
    SetBuzzer(BUZZER_ENABLE, freq=800, tps=0.2)
    time.sleep(0.6)

    while landed == 0:

        # Avant le décollage, on buzze et on chercher à détecter le décollage avec le GPS
        if launched == 0:
            
            # Buzzer jusqu'au décollage
            SetBuzzer(BUZZER_ENABLE, freq=1000, tps=0.2)
       
        
            # Détection du décollage
            if gnss_l76b.uart_any():
                sentence = parser.update(chr(gnss_l76b.uart_receive_byte()[0]))
                    
                if sentence:
                        
                    altitude = parser.altitude

                    if altitude - launch_altitude > 0:
                        
                        launched = 1
                        start_time = time.ticks_ms()


        # Une fois qu'on a décollé, on récupère les données du PMS et du GPS.
        if launched == 1:

            # Buzzer de vol
            SetBuzzer(BUZZER_ENABLE, freq=1500, tps=1)


            end_time = time.ticks_ms()
            execution_time = time.ticks_diff(end_time, start_time)/1000

            # Récupération des données GPS
            if gnss_l76b.uart_any():
                sentence = parser.update(chr(gnss_l76b.uart_receive_byte()[0]))
                
                if sentence:

                    sx.send(b"WGS84 Coordinate:Latitude(%c),Longitude(%c) %.9f,%.9f \n"%(parser.latitude[1],parser.longitude[1],parser.latitude[0],parser.longitude[0]))
                    
                    sx.send(b"UTC Timestamp:%d:%d:%d \n"%(parser.timestamp[0],parser.timestamp[1],parser.timestamp[2]))

                    new_altitude = parser.altitude
                    
                    PMS_data = str(pms5003.read())
                    print(PMS_data)

                    # On écrit les données sur la carte SD
                    file.write(str(execution_time) + "," + str(parser.latitude[1]) + "," + str(parser.latitude[0]) + "," + str(parser.longitude[1]) + "," + str(parser.longitude[0]) + "," + str(parser.timestamp[0]) + ":" + str(parser.timestamp[1]) + ":" + str(parser.timestamp[2]) + "," + str(parser.altitude) + "," + PMS_data + "\n")

                    file.flush()


                    # Détection de l'atterrissage
                    if abs(new_altitude - altitude) < 0.1:
                        count_landed += 1
                    else :
                        count_landed = 0
                    
                    new_altitude = altitude
                    
                    if count_landed > 10 :
                        landed = 1
                        file.close()

                    



    # Une fois qu'on a atterri, plus besoin d'enregistrer les données, on peut désactiver la carte SD.
    os.umount('/sd')

    #On se contente d'émettre la position toutes les 30 itérations de la boucle et de buzzer.

    SetBuzzer(BUZZER_ENABLE, freq=1500, tps=10)
    
    compteur = 0
    while True:
        
        if gnss_l76b.uart_any():
            
            sentence = parser.update(chr(gnss_l76b.uart_receive_byte()[0]))
            
            if sentence:

                if compteur == 30:

                    sx.send(b"WGS84 Coordinate:Latitude(%c),Longitude(%c) %.9f,%.9f \n"%(parser.latitude[1],parser.longitude[1],parser.latitude[0],parser.longitude[0]))
                
                    sx.send(b"UTC Timestamp:%d:%d:%d \n"%(parser.timestamp[0],parser.timestamp[1],parser.timestamp[2]))

                    compteur = 0

        compteur += 1
                



