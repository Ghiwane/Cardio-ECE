#include "affichage.h"
#include "memoire.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET -1
static Adafruit_SSD1306 ecran(LARGEUR_ECRAN, HAUTEUR_ECRAN, &Wire, OLED_RESET);
static uint8_t adresseActive = 0;
static uint8_t courbe[LARGEUR_ECRAN];
static uint8_t xCourant = 0;

// Langue de l'interface : 0 = Francais, 1 = English
static uint8_t langue = 0;

void definirLangue(uint8_t l) {
    langue = l;
}

// echelle choisie avec l'encodeur (definie dans main.cpp)
extern volatile uint8_t indexEchelle;
static unsigned long dernierAppelGraphe = 0;
static unsigned long resteMs = 0;
extern char __heap_start;
extern char *__brkval;
int ramLibre() {
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
void choisirEcran(uint8_t adresse) {
    if (adresse != adresseActive) {
        ecran.begin(SSD1306_SWITCHCAPVCC, adresse);
        adresseActive = adresse;
    }
    ecran.clearDisplay(); 
    ecran.setTextColor(SSD1306_WHITE);
}
void afficherAccueil(uint8_t indexLangue) {
    // Ecran 2 : Bonjour / Hello
    choisirEcran(ADRESSE_ECRAN2);
    ecran.setTextSize(2);
    ecran.setCursor(22, 14);
    ecran.print(F("Bonjour"));
    ecran.setCursor(34, 38);
    ecran.print(F("Hello"));
    ecran.display();

    // Ecran 1 : choix de la langue
    choisirEcran(ADRESSE_ECRAN1);
    ecran.setTextSize(1);
    ecran.setCursor(13, 2);
    ecran.print(F("Langue / Language"));

    // 1. Francais
    if (indexLangue == 0) {
        ecran.fillRect(3, 16, 122, 13, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 19);
    ecran.print(F("Fran"));
    ecran.write(0x87);          // c cedille (police par defaut, table CP437)
    ecran.print(F("ais"));

    // 2. English
    if (indexLangue == 1) {
        ecran.fillRect(3, 32, 122, 13, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 35);
    ecran.print(F("English"));

    ecran.display();
}

void afficherDateEcran(const char* dateStr) {
    choisirEcran(ADRESSE_ECRAN2);
    ecran.setTextSize(1);
    if (langue == 0) {
        ecran.setCursor(16, 15);
        ecran.print(F("Nous sommes le :"));
    } else {
        ecran.setCursor(37, 15);
        ecran.print(F("Today is:"));
    }
    ecran.setTextSize(2);
    ecran.setCursor(4, 38);
    ecran.print(dateStr);
    ecran.display();
    afficherMenuSelection(0);
}
//  Menu Sélection Personne + Retour + Quitter (6 choix) 
void afficherMenuSelection(uint8_t indexSelection) {
    choisirEcran(ADRESSE_ECRAN1);
    ecran.clearDisplay();
    ecran.setTextSize(1);
    // 1. Carmen
    if (indexSelection == 0) {
        ecran.fillRect(3, 1, 122, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 2);
    ecran.print(F("Carmen"));
    // 2. Ghiwane
    if (indexSelection == 1) {
        ecran.fillRect(3, 12, 122, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 13);
    ecran.print(F("Ghiwane"));
    // 3. Nicolas
    if (indexSelection == 2) {
        ecran.fillRect(3, 23, 122, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 24);
    ecran.print(F("Nicolas"));
    // 4. Marion
    if (indexSelection == 3) {
        ecran.fillRect(3, 34, 122, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 35);
    ecran.print(F("Marion"));
    // 5. Retour
    if (indexSelection == 4) {
        ecran.fillRect(3, 45, 122, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 46);
    if (langue == 0) {
        ecran.print(F("Retour"));
    } else {
        ecran.print(F("Back"));
    }
    // 6. Quitter
    if (indexSelection == 5) {
        ecran.fillRect(3, 56, 122, 7, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 56);
    if (langue == 0) {
        ecran.print(F("Quitter"));
    } else {
        ecran.print(F("Exit"));
    }
    ecran.display();
}
//  Menu Actions + Effacer + Retour + Quitter (5 choix) 
void afficherMenuAction(uint8_t indexAction) {
    choisirEcran(ADRESSE_ECRAN1);
    ecran.clearDisplay();
    ecran.setTextSize(1);
    // 1. Mesurer
    if (indexAction == 0) {
        ecran.fillRect(3, 2, 122, 11, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 4);
    if (langue == 0) {
        ecran.print(F("Mesurer"));
    } else {
        ecran.print(F("Measure"));
    }
    // 2. Historique
    if (indexAction == 1) {
        ecran.fillRect(3, 14, 122, 11, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 16);
    if (langue == 0) {
        ecran.print(F("Historique"));
    } else {
        ecran.print(F("History"));
    }
    // 3. Effacer l'historique (ET6.4)
    if (indexAction == 2) {
        ecran.fillRect(3, 26, 122, 11, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 28);
    if (langue == 0) {
        ecran.print(F("Effacer historique"));
    } else {
        ecran.print(F("Clear history"));
    }
    // 4. Retour
    if (indexAction == 3) {
        ecran.fillRect(3, 38, 122, 11, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 40);
    if (langue == 0) {
        ecran.print(F("Retour"));
    } else {
        ecran.print(F("Back"));
    }
    // 5. Quitter
    if (indexAction == 4) {
        ecran.fillRect(3, 50, 122, 11, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(6, 52);
    if (langue == 0) {
        ecran.print(F("Quitter"));
    } else {
        ecran.print(F("Exit"));
    }
    ecran.display();
}
void afficherTitreHistorique(uint8_t personneId) {
    choisirEcran(ADRESSE_ECRAN1);
    ecran.setTextSize(1);
    ecran.setCursor(6, 4);
    if (langue == 0) {
        ecran.print(F("Historique de : "));
    } else {
        ecran.print(F("History of: "));
    }
    ecran.setCursor(6, 16);
    ecran.setTextSize(2);
    
    if (personneId == 0) ecran.print(F("Carmen"));
    else if (personneId == 1) ecran.print(F("Ghiwane"));
    else if (personneId == 2) ecran.print(F("Nicolas"));
    else if (personneId == 3) ecran.print(F("Marion"));
    
    ecran.display();
}
//  Liste Historique + Retour + Quitter (5 choix) 
void afficherListeHistorique(uint8_t personneId, uint8_t indexHistoriqueSelection) {
    Mesure historique[3];
    recupererHistorique(personneId, historique);
    choisirEcran(ADRESSE_ECRAN2);
    ecran.clearDisplay();
    ecran.setTextSize(1);
    ecran.setCursor(4, 1);
    if (langue == 0) {
        ecran.print(F("Choix mesure:"));
    } else {
        ecran.print(F("Select reading:"));
    }
    // Ligne 0 : Mesure 1
    if (indexHistoriqueSelection == 0) {
        ecran.fillRect(2, 11, 124, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(4, 12);
    ecran.print(F("1: "));
    if (historique[0].valeur == 0 || historique[0].valeur == 255) {
        if (langue == 0) {
            ecran.print(F("Aucune"));
        } else {
            ecran.print(F("None"));
        }
    }
    else { ecran.print(historique[0].valeur); ecran.print(F(" BPM")); }
    // Ligne 1 : Mesure 2
    if (indexHistoriqueSelection == 1) {
        ecran.fillRect(2, 22, 124, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(4, 23);
    ecran.print(F("2: "));
    if (historique[1].valeur == 0 || historique[1].valeur == 255) {
        if (langue == 0) {
            ecran.print(F("Aucune"));
        } else {
            ecran.print(F("None"));
        }
    }
    else { ecran.print(historique[1].valeur); ecran.print(F(" BPM")); }
    // Ligne 2 : Mesure 3
    if (indexHistoriqueSelection == 2) {
        ecran.fillRect(2, 33, 124, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(4, 34);
    ecran.print(F("3: "));
    if (historique[2].valeur == 0 || historique[2].valeur == 255) {
        if (langue == 0) {
            ecran.print(F("Aucune"));
        } else {
            ecran.print(F("None"));
        }
    }
    else { ecran.print(historique[2].valeur); ecran.print(F(" BPM")); }
    // Ligne 3 : Retour
    if (indexHistoriqueSelection == 3) {
        ecran.fillRect(2, 44, 124, 10, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(4, 45);
    if (langue == 0) {
        ecran.print(F("Retour"));
    } else {
        ecran.print(F("Back"));
    }
    // Ligne 4 : Quitter
    if (indexHistoriqueSelection == 4) {
        ecran.fillRect(2, 55, 124, 8, SSD1306_WHITE);
        ecran.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        ecran.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    ecran.setCursor(4, 55);
    if (langue == 0) {
        ecran.print(F("Quitter"));
    } else {
        ecran.print(F("Exit"));
    }
    ecran.display();
}
void afficherDetailMesure(Mesure m, uint8_t personneId) {
    choisirEcran(ADRESSE_ECRAN1);
    ecran.setTextSize(1);
    ecran.setCursor(6, 4);
    if (langue == 0) {
        ecran.print(F("Detail mesure"));
    } else {
        ecran.print(F("Measurement detail"));
    }
    
    ecran.setTextSize(2);
    ecran.setCursor(10, 26);
    if (m.valeur == 0 || m.valeur == 255) {
        if (langue == 0) {
            ecran.print(F("Aucune"));
        } else {
            ecran.print(F("None"));
        }
    } else {
        ecran.print(m.valeur);
        ecran.print(F(" BPM"));
    }
    ecran.display();
    choisirEcran(ADRESSE_ECRAN2);
    ecran.setTextSize(1);
    ecran.setCursor(6, 6);
    if (langue == 0) {
        ecran.print(F("Date & Heure :"));
    } else {
        ecran.print(F("Date & Time:"));
    }
    ecran.setTextSize(1);
    ecran.setCursor(10, 26);
    if (m.jour == 0 || m.mois == 0) {
        if (langue == 0) {
            ecran.print(F("Date : --/--"));
        } else {
            ecran.print(F("Date: --/--"));
        }
    } else {
        if (langue == 0) {
            ecran.print(F("Date : "));
        } else {
            ecran.print(F("Date: "));
        }
        if (m.jour < 10) ecran.print('0');
        ecran.print(m.jour);
        ecran.print('/');
        if (m.mois < 10) ecran.print('0');
        ecran.print(m.mois);
    }
    ecran.setCursor(10, 44);
    if (langue == 0) {
        ecran.print(F("Heure: "));
    } else {
        ecran.print(F("Time: "));
    }
    if (m.heure < 10) ecran.print('0');
    ecran.print(m.heure);
    ecran.print(':');
    if (m.minute < 10) ecran.print('0');
    ecran.print(m.minute);
    ecran.display();
}
void afficherEcran1(uint8_t heure, uint8_t minute, uint8_t seconde, float bpm, EtatEnregistrement etat, uint8_t nbMesures) {
    choisirEcran(ADRESSE_ECRAN1);
    ecran.setTextSize(3);
    ecran.setCursor(5, 4);
    if (heure < 10) ecran.print('0');
    ecran.print(heure);
    ecran.print(':');
    if (minute < 10) ecran.print('0');
    ecran.print(minute);
    ecran.setTextSize(1);
    ecran.setCursor(98, 4);
    if (seconde < 10) ecran.print('0');
    ecran.print(seconde);
    ecran.print(F("s"));
    ecran.drawFastHLine(0, 32, LARGEUR_ECRAN, SSD1306_WHITE);
    ecran.setTextSize(2);
    ecran.setCursor(10, 38);
    if (bpm >= BPM_MIN && bpm <= BPM_MAX) ecran.print((int)(bpm + 0.5));
    else ecran.print('-');
    ecran.print(F(" bpm"));
    ecran.drawBitmap(115, 43, heart_bmp, 8, 8, SSD1306_WHITE);

    //progression de l'enregistrement sur la derniere ligne
    ecran.setTextSize(1);
    ecran.setCursor(4, 56);
    if (etat == ENREGISTREMENT_EN_COURS) {
        if (langue == 0) {
            ecran.print(F("Enregistrement "));
        } else {
            ecran.print(F("Recording "));
        }
        ecran.print(nbMesures);
        ecran.print('/');
        ecran.print(NB_MESURES_MOYENNE);
    } else if (etat == ENREGISTREMENT_TERMINE) {
        if (langue == 0) {
            ecran.print(F("Mesure enregistree"));
        } else {
            ecran.print(F("Reading saved"));
        }
    }
    ecran.display();
}
void afficherEcran2(int valeurPPG) {
    // Base de temps (ms par pixel), tout en if
    unsigned long msParPixel = 50;
    if (indexEchelle == 0) {
        msParPixel = 10;
    } else if (indexEchelle == 1) {
        msParPixel = 20;
    } else if (indexEchelle == 2) {
        msParPixel = 50;
    } else if (indexEchelle == 3) {
        msParPixel = 100;
    } else if (indexEchelle == 4) {
        msParPixel = 200;
    }

    // Temps ecoule depuis le dernier appel -> nombre de pixels a avancer
    unsigned long maintenant = millis();
    unsigned long ecoule = maintenant - dernierAppelGraphe;
    dernierAppelGraphe = maintenant;
    if (ecoule > 500) ecoule = 0;
    resteMs += ecoule;

    uint8_t yPoint = map(valeurPPG, 0, 1023, HAUTEUR_ECRAN - 1, HAUT_GRAPHE);
    while (resteMs >= msParPixel) {
        resteMs -= msParPixel;
        courbe[xCourant] = yPoint;
        xCourant++;
        if (xCourant >= LARGEUR_ECRAN) xCourant = 0;
    }

    choisirEcran(ADRESSE_ECRAN2);
    ecran.setTextSize(1);
    ecran.setCursor(0, 0);
    ecran.print(F("PPG "));
    if (indexEchelle == 0) {
        ecran.print(F("10ms/px"));
    } else if (indexEchelle == 1) {
        ecran.print(F("20ms/px"));
    } else if (indexEchelle == 2) {
        ecran.print(F("50ms/px"));
    } else if (indexEchelle == 3) {
        ecran.print(F("100ms/px"));
    } else if (indexEchelle == 4) {
        ecran.print(F("200ms/px"));
    }

    for (uint8_t x = 1; x < LARGEUR_ECRAN; x++) {
        if (courbe[x - 1] == 0 || courbe[x] == 0) continue;
        if (x == xCourant) continue;
        ecran.drawLine(x - 1, courbe[x - 1], x, courbe[x], SSD1306_WHITE);
    }
    ecran.display();
}
void afficherMessageQuitter() {
    choisirEcran(ADRESSE_ECRAN2);
    ecran.setTextSize(1);
    if (langue == 0) {
        ecran.setCursor(28, 22);
        ecran.print(F("Appuyer pour"));
        ecran.setCursor(43, 36);
        ecran.print(F("QUITTER"));
    } else {
        ecran.setCursor(40, 22);
        ecran.print(F("Press to"));
        ecran.setCursor(52, 36);
        ecran.print(F("QUIT"));
    }
    ecran.display();
}
void afficherAuRevoir() {
    choisirEcran(ADRESSE_ECRAN1);
    ecran.setTextSize(2);
    if (langue == 0) {
        ecran.setCursor(10, 24);
        ecran.print(F("Au revoir"));
    } else {
        ecran.setCursor(22, 24);
        ecran.print(F("Goodbye"));
    }
    ecran.display();
    choisirEcran(ADRESSE_ECRAN2);
    ecran.setTextSize(1);
    if (langue == 0) {
        ecran.setCursor(22, 22);
        ecran.print(F("A la prochaine"));
        ecran.setCursor(52, 36);
        ecran.print(F("fois"));
    } else {
        ecran.setCursor(43, 22);
        ecran.print(F("See you"));
        ecran.setCursor(37, 36);
        ecran.print(F("next time"));
    }
    ecran.display();
}
void eteindreEcrans() {
    choisirEcran(ADRESSE_ECRAN1);
    ecran.clearDisplay();
    ecran.display();
    choisirEcran(ADRESSE_ECRAN2);
    ecran.clearDisplay();
    ecran.display();
}