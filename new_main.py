import time
import math
import camera
import os
import machine
from machine import I2C, Pin
from bmp280 import *
import utime

i = None
alt = None
pres_suiv = None
press = None
nb_mesures = None
pas_de_temps = None
list2 = None
pressions = None
j = None
norm_pres = None
seuil = None
exp = None
p1 = None
en_descente = None
diff_press = None
au_sol = None
data = None
donnees = None
name = None
image = None

def upRange(start, stop, step):
  while start <= stop:
    yield start
    start += abs(step)

def downRange(start, stop, step):
  while start >= stop:
    yield start
    start -= abs(step)

# Permet de tester si on est au sol, en vérifiant
# si la pression reste constante au cours du temps
def test_au_sol():
  global i, alt, pres_suiv, press, nb_mesures, pas_de_temps, list2, pressions, j, norm_pres, seuil, exp, p1, en_descente, diff_press, au_sol, data, donnees, name, image
  i_end = float(nb_mesures - 1)
  for i in (1 <= i_end) and upRange(1, i_end, 1) or downRange(1, i_end, 1):
    pres_suiv = list2[int(i + 1)]
    list2[int(i)] = pres_suiv
  time.sleep(pas_de_temps)
  bmp280.normal_measure()
  list2[int(nb_mesures)] = bmp280.pressure
  j = 1
  while math.fabs(diff_press) <= seuil and j < nb_mesures:
    p1 = list2[int(j)]
    diff_press = p1 - list2[int(j + 1)]
    j = (j if isinstance(j, int) else 0) + 1
  if j == nb_mesures - 1:
    au_sol = True

# Describe this function...
def calcul_altitude():
  global i, alt, pres_suiv, press, nb_mesures, pas_de_temps, list2, pressions, j, norm_pres, seuil, exp, p1, en_descente, diff_press, au_sol, data, donnees, name, image
  alt = 0
  press = 0
  bmp280.normal_measure()
  press = bmp280.pressure
  norm_pres = press / 1013.25
  exp = math.exp(math.log(norm_pres * 0.19035))
  alt = 145442.1567 * (1 - exp)
  return alt

# Permet de tester si on descend, en vérifiant
# si la pression augmente au cours du temps
def test_descente():
  global i, alt, pres_suiv, press, nb_mesures, pas_de_temps, list2, pressions, j, norm_pres, seuil, exp, p1, en_descente, diff_press, au_sol, data, donnees, name, image
  i_end2 = float(nb_mesures - 1)
  for i in (1 <= i_end2) and upRange(1, i_end2, 1) or downRange(1, i_end2, 1):
    pres_suiv = list2[int(i + 1)]
    list2[int(i)] = pres_suiv
  time.sleep(pas_de_temps)
  bmp280.normal_measure()
  list2[int(nb_mesures)] = bmp280.pressure
  j = 1
  while diff_press <= 0 and math.fabs(diff_press) >= seuil and j < nb_mesures:
    p1 = list2[int(j)]
    diff_press = p1 - list2[int(j + 1)]
    j = (j if isinstance(j, int) else 0) + 1
  if j == nb_mesures - 1:
    en_descente = True


camera.init()
os.mount(machine.SDCard(), '/sd')
bus=I2C(scl=Pin(22), sda=Pin(21))
bmp280 = BMP280(bus)
bmp280.use_case(BMP280_CASE_WEATHER)
bmp280.oversample(BMP280_OS_HIGH)
nb_mesures = 5
pressions = [0] * nb_mesures
pas_de_temps = 0.3
seuil = 100
en_descente = False
au_sol = False
for i in pressions:
  bmp280.normal_measure()
  list2[int(i)] = bmp280.pressure
  time.sleep(pas_de_temps)
while True:
  test_descente()
  if en_descente == True:
    data = '/sd/data' + '.txt'
    donnees = open(data, 'a')

    donnees.write((''.join([str(x) for x in ['Temps (ms)', ',', 'Altitude (ft)', ',', 'Pression (Pa)', ',', 'Température (°C)']])))
    donnees.write('\n')
    while not au_sol:
      name = ''.join([str(x2) for x2 in ['/sd/img_', str((utime.ticks_ms())), '.jpg']])
      image = open(name, 'bw')

      image.write((camera.capture()))
      image.close()
      bmp280.normal_measure()
      donnees.write((''.join([str(x3) for x3 in [str((utime.ticks_ms())), ',', str((calcul_altitude())), ',', str((bmp280.pressure)), ',', str((bmp280.temperature))]])))
      donnees.write('\n')
      test_au_sol()
    donnees.close()
    os.umount('/sd')
    camera.deinit()
    bmp280.sleep()
