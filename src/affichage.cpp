// Test autonome des 2 écrans OLED avec UN SEUL objet Adafruit (méthode du prof)
// Un seul tampon de 1 Ko en RAM : on change d'écran en rappelant begin() avec l'autre adresse
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LARGEUR_ECRAN  128
#define HAUTEUR_ECRAN  64
#define OLED_RESET     -1    // pas de broche reset sur les modules I2C
#define ADRESSE_ECRAN1 0x3C  // heure + bpm
#define ADRESSE_ECRAN2 0x3D  // PPG
#define HAUT_GRAPHE    10    // le graphe PPG commence sous le titre
#define PIN_CAPTEUR    A0
#define BPM_MIN        30
#define BPM_MAX        220

static Adafruit_SSD1306 ecran(LARGEUR_ECRAN, HAUTEUR_ECRAN, &Wire, OLED_RESET);
static uint8_t adresseActive = 0;          // écran actuellement sélectionné

static uint8_t courbe[LARGEUR_ECRAN];      // ordonnée du PPG pour chaque colonne
static uint8_t xCourant = 0;           // prochaine colonne à remplir

// RAM libre entre le tas et la pile (spécifique AVR)
extern int __heap_start, *__brkval;
int ramLibre(){
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void choisirEcran(uint8_t adresse){
    if (adresse != adresseActive){
        // le tampon déjà alloué est réutilisé, seule l'adresse change
        ecran.begin(SSD1306_SWITCHCAPVCC, adresse);
        adresseActive = adresse;
    }
    ecran.clearDisplay();  // le tampon contient encore l'image de l'autre écran
    ecran.setTextColor(SSD1306_WHITE);
}

void afficherEcran1(uint8_t heure, uint8_t minute, float bpm){
    choisirEcran(ADRESSE_ECRAN1);

    // heure HH:MM en grand, centrée (5 caractères x 18 px = 90 px)
    ecran.setTextSize(3);
    ecran.setCursor(19, 4);
    if (heure < 10) ecran.print('0');
    ecran.print(heure);
    ecran.print(':');
    if (minute < 10) ecran.print('0');
    ecran.print(minute);

    ecran.drawFastHLine(0, 32, LARGEUR_ECRAN, SSD1306_WHITE);

    // bpm, ou un tiret si la valeur n'est pas réaliste
    ecran.setCursor(10, 38);
    if (bpm >= BPM_MIN && bpm <= BPM_MAX) ecran.print((int)(bpm + 0.5));
    else ecran.print('-');
    ecran.setTextSize(2);
    ecran.print(F(" bpm"));

    ecran.display();
}

void afficherEcran2(int valeurPPG){
    // on mémorise le nouveau point (0..1023 -> ligne de l'écran, 0 = haut)
    courbe[xCourant] = map(valeurPPG, 0, 1023, HAUTEUR_ECRAN - 1, HAUT_GRAPHE);
    xCourant++;
    if (xCourant >= LARGEUR_ECRAN) xCourant = 0;  // balayage : on repart à gauche

    choisirEcran(ADRESSE_ECRAN2);

    ecran.setTextSize(1);
    ecran.setCursor(0, 0);
    ecran.print(F("PPG"));

    // on redessine toute la courbe, sauf la jonction entre nouveaux et anciens points
    for (uint8_t x = 1; x < LARGEUR_ECRAN; x++){
        if (x == xCourant) continue;
        ecran.drawLine(x - 1, courbe[x - 1], x, courbe[x], SSD1306_WHITE);
    }

    ecran.display();
}

void setup(){
    Serial.begin(9600);
    delay(200);  // laisse le temps aux écrans de démarrer (comme dans le code du prof)

    for (uint8_t x = 0; x < LARGEUR_ECRAN; x++) courbe[x] = HAUTEUR_ECRAN - 1;

    Serial.print(F("RAM libre avant l'ecran : "));
    Serial.println(ramLibre());

    afficherEcran1(9, 53, 72);
    afficherEcran2(0);

    Serial.print(F("RAM libre apres l'ecran : "));
    Serial.println(ramLibre());
}

void loop(){
    static unsigned long dernierEcran1 = 0;

    // écran 1 rafraîchi une fois par seconde (un display() prend ~25 ms)
    if (millis() - dernierEcran1 >= 1000){
        dernierEcran1 = millis();
        afficherEcran1(9, 53, 72);  // heure et bpm fixes pour le test
    }

    afficherEcran2(analogRead(PIN_CAPTEUR));
}