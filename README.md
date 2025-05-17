#  Dossier Technique – Station Météo Connectée

##  Sujet choisi

**Sujet 2 – Station Météo Connectée**

---

##  Identification des problèmes dans le contexte

- Récupérer et afficher les données météorologiques

---

##  Proposition de réponse

- Mise en place de capteurs afin de créer une station météorologique
- Exploitation des données via interface visuelle

---

##  Composants utilisés

###  Microcontrôleurs
- ESP32  
- Breadboard  
- Câble micro-USB

###  Capteurs
- Module à effet Hall **KY024LM**
- Capteur **BME280** : pression, température, humidité
- **Luxmètre SEN0097**

###  Actionneurs
- Ventilateur **Gravity DFR0332**
- Bandeau de LED **Stick NeoPixel RGB 8 LEDs ADA1426**
- Afficheur **OLED 0,96" I2C OLED01**

###  Divers
- Aimant  
- Prise avec alimentation (1V – 12V)  
- Connecteur pour prise alimentation ou coupleur 4 piles LR3 (4AAA-P)  
- Charnière fer  
- Câbles  
- Module **Grove Base Shield 103030000**  
- Bouton poussoir  
- Résistances  
- Autres composants selon besoins

---

##  Répartition des tâches au sein du groupe

| Prénom   | Rôle                                                       |
|----------|------------------------------------------------------------|
| Jean     | Conception hardware et câblage                             |
| Nikola   | Développement logiciel pour capteurs et actionneurs       |
| Mathéo   | *(À compléter)*                                            |
| Kylian   | *(À compléter)*                                            |

---

##  Outils utilisés

- [GitHub](https://github.com/GeantCreeper/weather_station) – gestion du code source
- Discord – communication de groupe
- Visual Studio Code – développement logiciel
- Node-RED – mise en place de la logique pour la Station Météorologique
- Arduino IDE – injection du code dans l'ESP32

---

##  Architecture logicielle

>  **Logigramme à insérer ici**

---

##  Schéma de câblage

>  **Schéma à insérer ici (format image ou Fritzing)**

---

##  Intégration de Node-RED

Node-RED est utilisé pour :
- **Récupérer les données météo à distance**
- **Afficher les données sur un tableau de bord dynamique**
- **Créer des interactions ou alertes selon les conditions météo**

---

##  Maquette et modèles 3D

- [Structure de la Station Météorologique](https://boxes.hackerspace-bamberg.de/Console2?language=fr)
- [Support vertical pour ESP32](https://www.printables.com/model/537566-customizable-esp32-lateral-support)
- [Support horizontal pour ESP32](https://www.thingiverse.com/thing:4230273)
- [Anémomètre 1](https://www.printables.com/model/599533-anemometer-v2-for-arduino) ou [Anémomètre 2](https://www.thingiverse.com/thing:942299) ou [Anémomètre 3](https://www.thingiverse.com/thing:3648443)
