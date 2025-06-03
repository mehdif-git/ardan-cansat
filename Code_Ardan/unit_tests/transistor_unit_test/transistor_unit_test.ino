#define TRANSISTOR_PIN 12  // Broche GPIO 12 connectée à la base du transistor

void setup() {
  
  // Configuration de la broche du transistor en sortie
  pinMode(TRANSISTOR_PIN, OUTPUT);
  
  // Assurez-vous que le transistor est désactivé au démarrage
  digitalWrite(TRANSISTOR_PIN, LOW);
}

void loop() {
  digitalWrite(TRANSISTOR_PIN, HIGH);
  delay(2000);  // Attendre 2 secondes
  
  digitalWrite(TRANSISTOR_PIN, LOW);
  delay(3000);  // Attendre 3 secondes
}