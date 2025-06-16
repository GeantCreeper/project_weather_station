//Tout d'abord, vous devez importer les bibliothèques nécessaires. 
//La bibliothèque Wire pour utiliser le protocole I2C et les bibliothèques Adafruit pour écrire sur l'écran
#include <Wire.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Définir la taille de l'écran OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Créer un objet de la classe Adafruit_SSD1306
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {

  Wire.begin(); // Initialiser la communication I2C avec l'écran OLED
  Serial.begin(9600); //Démarre la communication série

  //vérifie si l'initialisation de l'écran OLED SSD1306 avec l'adresse 0x3C a réussi
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);

  }
  delay(2000);

  // Effacer tout l'écran
  display.clearDisplay();
  display.setTextSize(2);// Définir la taille du texte
  display.setTextColor(WHITE); // Définir la couleur de la police
  display.setCursor(32, 32); // Définir la position du texte
  display.println(""); // Envoyer le texte à afficher
  display.display();  // Afficher le texte sur l'écran
}
void loop() {
  //// Aucun code dans cette partie, car tout se passe dans la fonction setup !
}
