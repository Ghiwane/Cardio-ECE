#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "capteur.h"
#include "horloge.h"
#include "etatSante.h"
#include "buzzer.h"
#include "affichage.h"
#include "memoire.h"

// Un etat = un ecran ou un menu
enum EtatSysteme {
    ATTENTE_ACCUEIL,
    CHOIX_LANGUE,
    ATTENTE_DATE,
    MENU_ACTION,
    AFFICHAGE_HISTORIQUE,
    AFFICHAGE_DETAIL_HISTORIQUE,
    MESURE_ACTIVE,
    ATTENTE_CONFIRMATION_QUITTER,
    QUITTER
};
static EtatSysteme etat = ATTENTE_ACCUEIL;

// Menus : indexLangue 0 = FR / 1 = EN ; indexSelection 0-3 personnes, 4 Retour, 5 Quitter ;
// indexAction 0 Mesurer, 1 Historique, 2 Effacer, 3 Retour, 4 Quitter ;
// indexHistoriqueSelection 0-2 mesures, 3 Retour, 4 Quitter
static uint8_t indexLangue = 0;
static uint8_t indexSelection = 0;
static uint8_t indexAction = 0;
static uint8_t indexHistoriqueSelection = 0;
static uint8_t personneChoisie = 0;
static unsigned long chronoMenuQuitter = 0;
static unsigned long chronoQuitter = 0;

// ET6.2 : enregistrement = moyenne de 10 mesures consecutives (1 par seconde)
static EtatEnregistrement etatEnregistrement = AUCUN_ENREGISTREMENT;
static float sommeBpm = 0;
static uint8_t nbMesures = 0;

// ET5.4 : encodeur lu par interruption sur D8 (PCINT0). L'ISR compte les crans, loop() les recupere.
volatile uint8_t indexEchelle = 2;           // 0 a 4 = 10, 20, 50, 100, 200 ms/px (lu par affichage.cpp)
static const uint8_t NB_ECHELLES = 5;
static volatile int8_t cransEncodeur = 0;    // +1 par cran horaire, -1 par cran anti-horaire
static uint8_t dernierCran = 0;

ISR(PCINT0_vect) {
    if (digitalRead(PIN_ENCODEUR_CLK) == LOW) {
        return;                                          // front montant seulement
    }
    uint8_t t = (uint8_t)millis();
    if ((uint8_t)(t - dernierCran) < 5) {
        return;                                          // anti-rebond 5 ms
    }
    dernierCran = t;
    if (digitalRead(PIN_ENCODEUR_DT) == LOW) {
        cransEncodeur++;
    } else {
        cransEncodeur--;
    }
}

// Boutons avec anti-rebond sans delay : true une seule fois par appui
struct Bouton {
    uint8_t pin;
    int etatPrecedent;
    unsigned long dernierChangement;
};
static Bouton boutonMenu        = {PIN_BOUTON_MENU, HIGH, 0};
static Bouton boutonEnregistrer = {PIN_BOUTON_ENREGISTRER, HIGH, 0};

static bool boutonAppuye(Bouton &b) {
    int etatBouton = digitalRead(b.pin);
    if (etatBouton == b.etatPrecedent || millis() - b.dernierChangement <= DUREE_MIN_REBOND) {
        return false;
    }
    b.etatPrecedent = etatBouton;
    b.dernierChangement = millis();
    return etatBouton == LOW;
}

// Deplacement circulaire dans un menu de nbChoix lignes
static uint8_t naviguer(uint8_t index, int sens, uint8_t nbChoix) {
    return (index + nbChoix + sens) % nbChoix;
}

// Ecran 2 : date du jour, ecran 1 : menu de choix de la personne
static void ouvrirSelection() {
    char dateDuJour[11];
    horlogeDateTexte(dateDuJour);
    afficherDateEcran(dateDuJour);
    afficherMenuSelection(indexSelection);
}

static void ouvrirHistorique() {
    indexHistoriqueSelection = 0;
    afficherTitreHistorique(personneChoisie);
    afficherListeHistorique(personneChoisie, 0);
    etat = AFFICHAGE_HISTORIQUE;
}

static void quitter() {
    afficherAuRevoir();
    chronoQuitter = millis();
    etat = QUITTER;
}

// Ecran 1 rafraichi chaque seconde (ou tout de suite si immediat).
// Pendant un enregistrement : une mesure par seconde.
static void rafraichirEcran1(bool immediat) {
    static unsigned long dernierEcran1 = 0;
    if (!immediat && millis() - dernierEcran1 < 1000) {
        return;
    }
    dernierEcran1 = millis();

    if (etatEnregistrement == ENREGISTREMENT_EN_COURS) {
        if (ppgBpmValide) {
            sommeBpm += ppgBpm;
            nbMesures++;
        } else {                                         // mesures consecutives : on recommence
            sommeBpm = 0;
            nbMesures = 0;
        }
        if (nbMesures == NB_MESURES_MOYENNE) {           // ET6.2 / ET6.3 : moyenne + date + heure du RTC
            uint8_t moyenne = (uint8_t)(sommeBpm / NB_MESURES_MOYENNE + 0.5);
            ajouterMesure(personneChoisie, moyenne,
                          maintenant.heure, maintenant.minute, maintenant.jour, maintenant.mois);
            etatEnregistrement = ENREGISTREMENT_TERMINE;
        }
    }
    afficherEcran1(maintenant.heure, maintenant.minute, maintenant.seconde,
                   ppgBpm, etatEnregistrement, nbMesures);
}

void setup() {
    delay(200);                                  // laisse les ecrans demarrer
    pinMode(PIN_BOUTON_MENU, INPUT_PULLUP);
    pinMode(PIN_BOUTON_ENREGISTRER, INPUT_PULLUP);
    pinMode(PIN_ENCODEUR_CLK, INPUT_PULLUP);
    pinMode(PIN_ENCODEUR_DT, INPUT_PULLUP);
    PCICR  |= (1 << PCIE0);                      // interruptions du port B...
    PCMSK0 |= (1 << PCINT0);                     // ... uniquement sur D8 (encodeur CLK)
    horlogeInit();
    etatSanteInit();
    buzzerInit();
    Wire.setWireTimeout(25000, true);            // un ecran mal branche ne bloque pas le programme
    afficherAccueil(indexLangue);
    etat = CHOIX_LANGUE;
}

void loop() {
    // Taches permanentes, quel que soit l'ecran affiche
    int valeurPPG = ppgTraiter();                // FS1
    horlogeMettreAJour();                        // FS2
    afficherEtatSante(ppgBpm);                   // FS3
    etatBuzzer(digitalRead(PIN_BOUTON_BEEP));    // FS4
    gererBuzzer(ppgBpm);

    // Entrees utilisateur
    bool appuiMenu = boutonAppuye(boutonMenu);
    bool appuiEnregistrer = boutonAppuye(boutonEnregistrer);

    noInterrupts();
    int8_t crans = cransEncodeur;                // crans depuis le dernier tour
    cransEncodeur = 0;
    interrupts();
    int sens = (crans > 0) - (crans < 0);        // +1, -1 ou 0 pour les menus

    switch (etat) {

        case ATTENTE_ACCUEIL:
            if (appuiMenu) {
                afficherAccueil(indexLangue);
                etat = CHOIX_LANGUE;
            }
            break;

        case CHOIX_LANGUE:
            if (sens != 0) {
                indexLangue = 1 - indexLangue;
                afficherAccueil(indexLangue);
            }
            if (appuiMenu) {
                definirLangue(indexLangue);
                indexSelection = 0;
                ouvrirSelection();
                etat = ATTENTE_DATE;
            }
            break;

        case ATTENTE_DATE:
            if (sens != 0) {
                indexSelection = naviguer(indexSelection, sens, 6);
                afficherMenuSelection(indexSelection);
            }
            if (!appuiMenu) {
                break;
            }
            if (indexSelection < 4) {                        // une personne
                personneChoisie = indexSelection;
                indexAction = 0;
                afficherMenuAction(indexAction);
                etat = MENU_ACTION;
            } else if (indexSelection == 4) {                // Retour
                afficherAccueil(indexLangue);
                etat = CHOIX_LANGUE;
            } else {                                         // Quitter
                quitter();
            }
            break;

        case MENU_ACTION:
            if (sens != 0) {
                indexAction = naviguer(indexAction, sens, 5);
                afficherMenuAction(indexAction);
            }
            if (!appuiMenu) {
                break;
            }
            if (indexAction == 0) {                          // Mesurer
                etatEnregistrement = AUCUN_ENREGISTREMENT;
                rafraichirEcran1(true);
                etat = MESURE_ACTIVE;
            } else if (indexAction == 1) {                   // Historique
                ouvrirHistorique();
            } else if (indexAction == 2) {                   // Effacer l'historique
                effacerHistorique(personneChoisie);
                ouvrirHistorique();                          // on montre l'historique vide
            } else if (indexAction == 3) {                   // Retour
                indexSelection = personneChoisie;
                ouvrirSelection();
                etat = ATTENTE_DATE;
            } else {                                         // Quitter
                quitter();
            }
            break;

        case AFFICHAGE_HISTORIQUE:
            if (sens != 0) {
                indexHistoriqueSelection = naviguer(indexHistoriqueSelection, sens, 5);
                afficherListeHistorique(personneChoisie, indexHistoriqueSelection);
            }
            if (!appuiMenu) {
                break;
            }
            if (indexHistoriqueSelection < 3) {              // une mesure
                Mesure historique[3];
                recupererHistorique(personneChoisie, historique);
                Mesure m = historique[indexHistoriqueSelection];
                if (m.valeur != 0 && m.valeur != 255) {      // on ouvre seulement une case remplie
                    afficherDetailMesure(m, personneChoisie);
                    etat = AFFICHAGE_DETAIL_HISTORIQUE;
                }
            } else if (indexHistoriqueSelection == 3) {      // Retour
                afficherMenuAction(indexAction);
                etat = MENU_ACTION;
            } else {                                         // Quitter
                quitter();
            }
            break;

        case AFFICHAGE_DETAIL_HISTORIQUE:
            if (appuiMenu) {
                afficherTitreHistorique(personneChoisie);
                afficherListeHistorique(personneChoisie, indexHistoriqueSelection);
                etat = AFFICHAGE_HISTORIQUE;
            }
            break;

        case MESURE_ACTIVE:
            if (appuiMenu) {
                afficherMessageQuitter();
                chronoMenuQuitter = millis();
                etat = ATTENTE_CONFIRMATION_QUITTER;
                break;
            }
            // ET5.4 : l'encodeur change l'echelle de temps de la courbe
            indexEchelle = constrain((int)indexEchelle + crans, 0, NB_ECHELLES - 1);
            // ET6.2 : le bouton ENREGISTRER lance (ou relance) un enregistrement
            if (appuiEnregistrer) {
                sommeBpm = 0;
                nbMesures = 0;
                etatEnregistrement = ENREGISTREMENT_EN_COURS;
            }
            rafraichirEcran1(false);
            afficherEcran2(valeurPPG);
            break;

        case ATTENTE_CONFIRMATION_QUITTER:
            rafraichirEcran1(false);                         // l'enregistrement continue
            if (appuiMenu) {
                quitter();                                   // un enregistrement non termine est abandonne
            } else if (millis() - chronoMenuQuitter >= 5000) {
                etat = MESURE_ACTIVE;                        // pas de confirmation : on reprend
            }
            break;

        case QUITTER:
            if (millis() - chronoQuitter >= 10000) {
                eteindreEcrans();
                etat = ATTENTE_ACCUEIL;                      // un nouvel appui rallume
            }
            break;
    }
}