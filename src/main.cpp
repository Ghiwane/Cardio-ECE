#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "capteur.h"
#include "horloge.h"
#include "etatSante.h"
#include "buzzer.h"
#include "affichage.h"
#include "memoire.h"

// ET5.4 : echelle de temps du graphe (0 a 4 = 10, 20, 50, 100, 200 ms/px)
volatile uint8_t indexEchelle = 2;
const int nbEchelles = 5;

enum EtatSysteme {
    ATTENTE_ACCUEIL,               // Ecrans eteints, attente d'un appui
    CHOIX_LANGUE,                  // Choix Francais / English
    ATTENTE_DATE,                  // Date + choix de la personne
    MENU_ACTION,
    AFFICHAGE_HISTORIQUE,          // Sélection des 3 mesures de la personne
    AFFICHAGE_DETAIL_HISTORIQUE,   // Valeur + Date + Heure
    MESURE_ACTIVE,
    ATTENTE_CONFIRMATION_QUITTER,
    QUITTER
};
static EtatSysteme etatCourant = ATTENTE_ACCUEIL;
// Variables pour l'encodeur et les menus
static uint8_t indexSelection = 0;           // Choix de la personne (0 à 3), Retour, Quitter
static uint8_t indexAction = 0;              // Menu d'actions (0 à 4)
static uint8_t indexHistoriqueSelection = 0; // L'une des 3 mesures, Retour, Quitter
static uint8_t indexLangue = 0;              // 0 = Francais, 1 = English
static uint8_t personneChoisie = 0;          // Mémorise la personne validée
static unsigned long chronoMenuQuitter = 0;
static unsigned long chronoQuitter = 0;
// ET6.2 : enregistrement lancé par le bouton ENREGISTRER (une mesure = le bpm relevé chaque seconde)
static EtatEnregistrement etatEnregistrement = AUCUN_ENREGISTREMENT;
static float sommeBpm = 0;
static uint8_t nbMesures = 0;

// ET5.4 : lecture de l'encodeur par interruption.
// En mesure, la boucle passe son temps dans display() : par scrutation on rate des crans
// et on lit mal le sens. L'interruption, elle, voit chaque cran.
// D2 et D3 (INT0/INT1) sont pris par les LEDs : on utilise l'interruption de changement
// d'état du port B (PCINT0 = broche D8 = PIN_ENCODEUR_CLK).
static uint8_t dernierTempsEncodeur = 0;

ISR(PCINT0_vect) {
    if (etatCourant != MESURE_ACTIVE) return;
    if (digitalRead(PIN_ENCODEUR_CLK) == LOW) return;      // on ne garde que le front montant
    uint8_t t = (uint8_t)millis();
    if ((uint8_t)(t - dernierTempsEncodeur) < 5) return;   // anti-rebond 5 ms
    dernierTempsEncodeur = t;
    if (digitalRead(PIN_ENCODEUR_DT) == LOW) {
        if (indexEchelle < nbEchelles - 1) indexEchelle++;
    } else {
        if (indexEchelle > 0) indexEchelle--;
    }
}

// Bouton du menu : renvoie true une seule fois par appui (anti-rebond sans bloquer la boucle)
bool boutonAppuye() {
    static int etatPrecedent = HIGH;
    static unsigned long dernierChangement = 0;
    int etatBouton = digitalRead(PIN_BOUTON_MENU);
    bool appui = false;

    if (etatBouton != etatPrecedent && millis() - dernierChangement > DUREE_MIN_REBOND) {
        appui = (etatBouton == LOW);
        etatPrecedent = etatBouton;
        dernierChangement = millis();
    }
    return appui;
}

// Bouton d'enregistrement (ET6.2) : même anti-rebond que boutonAppuye()
bool boutonEnregistrerAppuye() {
    static int etatPrecedent = HIGH;
    static unsigned long dernierChangement = 0;
    int etatBouton = digitalRead(PIN_BOUTON_ENREGISTRER);
    bool appui = false;

    if (etatBouton != etatPrecedent && millis() - dernierChangement > DUREE_MIN_REBOND) {
        appui = (etatBouton == LOW);
        etatPrecedent = etatBouton;
        dernierChangement = millis();
    }
    return appui;
}

// Encodeur lu par scrutation dans les menus : +1, -1 ou 0 cran
int lireEncodeur() {
    static int dernierEtatCLK = HIGH;
    int etatCLK = digitalRead(PIN_ENCODEUR_CLK);
    int sens = 0;

    if (etatCLK != dernierEtatCLK && etatCLK == HIGH) {
        if (digitalRead(PIN_ENCODEUR_DT) != etatCLK) sens = 1;
        else sens = -1;
    }
    dernierEtatCLK = etatCLK;
    return sens;
}

// Déplacement circulaire dans un menu de nbChoix lignes
uint8_t naviguer(uint8_t index, int sens, uint8_t nbChoix) {
    return (index + nbChoix + sens) % nbChoix;
}

// Date du jour "JJ/MM/AAAA" lue sur le module RTC
void afficherDate() {
    char dateDuJour[11];
    horlogeDateTexte(dateDuJour);
    afficherDateEcran(dateDuJour);
}

void ouvrirHistorique() {
    indexHistoriqueSelection = 0;
    afficherTitreHistorique(personneChoisie);
    afficherListeHistorique(personneChoisie, indexHistoriqueSelection);
    etatCourant = AFFICHAGE_HISTORIQUE;
}

void quitter() {
    afficherAuRevoir();
    chronoQuitter = millis();
    etatCourant = QUITTER;
}

// Ecran 1 (heure + bpm) rafraîchi chaque seconde ; pendant un enregistrement, on en profite pour relever une mesure (ET6.2)
void rafraichirEcran1() {
    static unsigned long dernierEcran1 = 0;
    if (millis() - dernierEcran1 < 1000) return;
    dernierEcran1 = millis();

    if (etatEnregistrement == ENREGISTREMENT_EN_COURS) {
        if (ppgBpmValide) {
            sommeBpm += ppgBpm;
            nbMesures++;
        } else {               // les mesures doivent être consécutives : on recommence
            sommeBpm = 0;
            nbMesures = 0;
        }
        // ET6.2 / ET6.3 : la moyenne des 10 mesures est enregistrée avec la date et l'heure du module RTC
        if (nbMesures == NB_MESURES_MOYENNE) {
            uint8_t moyenne = (uint8_t)(sommeBpm / NB_MESURES_MOYENNE + 0.5);
            ajouterMesure(personneChoisie, moyenne, maintenant.heure, maintenant.minute, maintenant.jour, maintenant.mois);
            etatEnregistrement = ENREGISTREMENT_TERMINE;
        }
    }
    afficherEcran1(maintenant.heure, maintenant.minute, maintenant.seconde, ppgBpm, etatEnregistrement, nbMesures);
}

void setup() {
    delay(200);  // laisse le temps aux écrans de démarrer
    pinMode(PIN_BOUTON_MENU, INPUT_PULLUP);
    pinMode(PIN_BOUTON_ENREGISTRER, INPUT_PULLUP);
    pinMode(PIN_ENCODEUR_CLK, INPUT_PULLUP);
    pinMode(PIN_ENCODEUR_DT, INPUT_PULLUP);
    PCICR  |= (1 << PCIE0);    // autorise les interruptions de changement d'état du port B
    PCMSK0 |= (1 << PCINT0);   // ... uniquement sur D8 (encodeur CLK)
    horlogeInit();
    etatSanteInit();
    buzzerInit();

    // I2C : si un écran est mal branché, on abandonne au bout de 25 ms au lieu de bloquer tout le programme
    Wire.setWireTimeout(25000, true);
    // On affiche tout de suite le choix de la langue : sinon les écrans gardent l'image de l'ancien programme
    afficherAccueil(indexLangue);
    etatCourant = CHOIX_LANGUE;
}

void loop() {
    // Tâches faites en permanence, quel que soit l'écran affiché
    int valeurPPG = ppgTraiter();                 // FS1 : met à jour ppgBpm
    horlogeMettreAJour();                         // FS2 : met à jour maintenant
    afficherEtatSante(ppgBpm);                    // FS3
    etatBuzzer(digitalRead(PIN_BOUTON_BEEP));     // FS4
    gererBuzzer(ppgBpm);

    bool appui = boutonAppuye();
    bool appuiEnregistrer = boutonEnregistrerAppuye();
    int sens = lireEncodeur();

    switch (etatCourant) {
        case ATTENTE_ACCUEIL:
            if (appui) {
                afficherAccueil(indexLangue);
                etatCourant = CHOIX_LANGUE;
            }
            break;
        case CHOIX_LANGUE:
            // L'encodeur alterne Francais / English
            if (sens != 0) {
                indexLangue = 1 - indexLangue;
                afficherAccueil(indexLangue);
            }
            // Appui : valide la langue puis passe a la date et au choix des personnes
            if (appui) {
                definirLangue(indexLangue);
                indexSelection = 0;
                afficherDate();
                etatCourant = ATTENTE_DATE;
            }
            break;
        case ATTENTE_DATE:
            if (sens != 0) {
                indexSelection = naviguer(indexSelection, sens, 6); // 0 à 3: personnes, 4: Retour, 5: Quitter
                afficherMenuSelection(indexSelection);
            }
            if (appui) {
                if (indexSelection < 4) {
                    personneChoisie = indexSelection;
                    indexAction = 0;
                    afficherMenuAction(indexAction);
                    etatCourant = MENU_ACTION;
                }
                else if (indexSelection == 4) {
                    afficherAccueil(indexLangue);
                    etatCourant = CHOIX_LANGUE;
                }
                else quitter();
            }
            break;
        case MENU_ACTION:
            if (sens != 0) {
                indexAction = naviguer(indexAction, sens, 5); // 0: Mesurer, 1: Historique, 2: Effacer, 3: Retour, 4: Quitter
                afficherMenuAction(indexAction);
            }
            if (appui) {
                if (indexAction == 0) {
                    etatEnregistrement = AUCUN_ENREGISTREMENT;   // nouvelle séance : rien en cours
                    afficherEcran1(maintenant.heure, maintenant.minute, maintenant.seconde, ppgBpm, etatEnregistrement, nbMesures);
                    etatCourant = MESURE_ACTIVE;
                }
                else if (indexAction == 1) ouvrirHistorique();
                else if (indexAction == 2) {
                    effacerHistorique(personneChoisie);
                    ouvrirHistorique();    // on montre l'historique vide
                }
                else if (indexAction == 3) {
                    indexSelection = personneChoisie;
                    afficherDate();
                    afficherMenuSelection(indexSelection);
                    etatCourant = ATTENTE_DATE;
                }
                else quitter();
            }
            break;
        case AFFICHAGE_HISTORIQUE:
            if (sens != 0) {
                indexHistoriqueSelection = naviguer(indexHistoriqueSelection, sens, 5); // 0-2: mesures, 3: Retour, 4: Quitter
                afficherListeHistorique(personneChoisie, indexHistoriqueSelection);
            }
            if (appui) {
                if (indexHistoriqueSelection < 3) {
                    Mesure historique[3];
                    recupererHistorique(personneChoisie, historique);
                    Mesure m = historique[indexHistoriqueSelection];
                    if (m.valeur != 0 && m.valeur != 255) {
                        afficherDetailMesure(m, personneChoisie);
                        etatCourant = AFFICHAGE_DETAIL_HISTORIQUE;
                    }
                }
                else if (indexHistoriqueSelection == 3) {
                    afficherMenuAction(indexAction);
                    etatCourant = MENU_ACTION;
                }
                else quitter();
            }
            break;
        case AFFICHAGE_DETAIL_HISTORIQUE:
            if (appui) {
                afficherTitreHistorique(personneChoisie);
                afficherListeHistorique(personneChoisie, indexHistoriqueSelection);
                etatCourant = AFFICHAGE_HISTORIQUE;
            }
            break;
        case MESURE_ACTIVE:
            if (appui) {
                afficherMessageQuitter();
                chronoMenuQuitter = millis();
                etatCourant = ATTENTE_CONFIRMATION_QUITTER;
                break;
            }
            // ET6.2 : le bouton ENREGISTRER lance (ou relance) un enregistrement de 10 mesures consécutives
            if (appuiEnregistrer) {
                sommeBpm = 0;
                nbMesures = 0;
                etatEnregistrement = ENREGISTREMENT_EN_COURS;
            }
            rafraichirEcran1();
            afficherEcran2(valeurPPG);
            break;
        case ATTENTE_CONFIRMATION_QUITTER:
            rafraichirEcran1();
            if (appui) {
                quitter();             // un enregistrement non terminé est abandonné
            }
            else if (millis() - chronoMenuQuitter >= 5000) {
                etatCourant = MESURE_ACTIVE;
            }
            break;
        case QUITTER:
            if (millis() - chronoQuitter >= 10000) {
                eteindreEcrans();
                etatCourant = ATTENTE_ACCUEIL;   // un nouvel appui rallume
            }
            break;
    }
}
