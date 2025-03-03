import time
import math
import camera
import os
import machine
from machine import I2C, Pin
from bmp280 import *
import utime

# Permet de tester si on est au sol, en vérifiant
# si la pression reste constante au cours du temps
def test_au_sol():
  mesures_press = []
  for i in range(nb_mesures):
    bmp280.normal_measure()
    mesures_press.append(bmp280.pressure)
    time.sleep(pas_de_temps)
  
  pression_ref = mesures_press[0] 
  
  au_sol = all(abs(press-pression_ref) < seuil for press in mesures_press)
  return au_sol

# Calcul de l'altitude (en pieds) à partir de la pression ambiante
def calcul_altitude(): 
  bmp280.normal_measure()
  press = bmp280.pressure
  norm_pres = press / 1013.25
  exp = norm_pres ** 0.19035
  alt = 145442.1567 * (1 - exp)
  return alt

# Permet de tester si on descend, en vérifiant
# si la pression augmente au cours du temps
def test_descente():
  mesures_press = []
  for i in range(nb_mesures):
    bmp280.normal_measure()
    mesures_press.append(bmp280.pressure)
    time.sleep(pas_de_temps)
  
  pression_ref = mesures_press[0] 
  
  en_descente = all(abs(press-pression_ref) > seuil_descente for press in mesures_press)
  return en_descente


camera.init()
os.mount(machine.SDCard(), '/sd')
bus=I2C(scl=Pin(22), sda=Pin(21))
bmp280 = BMP280(bus)
bmp280.use_case(BMP280_CASE_WEATHER)
bmp280.oversample(BMP280_OS_HIGH)

# Entier
nb_mesures = 5

# En secondes
pas_de_temps = 0.3

# En Pa
seuil_cst = 100
seuil_descente = 50

while True:

  if test_descente():
    data = '/sd/data' + '.txt'
    donnees = open(data, 'a')

    donnees.write((''.join([str(x) for x in ['Temps (ms)', ',', 'Altitude (ft)', ',', 'Pression (Pa)', ',', 'Température (°C)']])))
    donnees.write('\n')
    while not test_au_sol():
      name = ''.join([str(x2) for x2 in ['/sd/img_', str((utime.ticks_ms())), '.jpg']])
      image = open(name, 'bw')
      image.write((camera.capture()))
      image.close()
      
      bmp280.normal_measure()
      donnees.write((''.join([str(x3) for x3 in [str((utime.ticks_ms())), ',', str((calcul_altitude())), ',', str((bmp280.pressure)), ',', str((bmp280.temperature))]])))
      donnees.write('\n')
    donnees.close()
    os.umount('/sd')
    camera.deinit()
    bmp280.sleep()