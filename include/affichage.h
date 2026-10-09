#ifndef AFFICHAGE_H 
#define AFFICHAGE_H

#include <Arduino.h>
#include "config.h"
#include "memoire.h"

#define LARGEUR_ECRAN  128
#define HAUTEUR_ECRAN  64
#define ADRESSE_ECRAN1 0x3C  // Adresse de l'écran 1 (Heure + BPM)
#define ADRESSE_ECRAN2 0x3D  // Adresse de l'écran 2 (PPG)
#define HAUT_GRAPHE    10    // Le graphe PPG commence sous le titre

// Définition du motif Cœur fixe (8x8 pixels)
const unsigned char PROGMEM heart_bmp[] = {    
    0b01100110,    
    0b11111111,    
    0b11111111,    
    0b11111111,    
    0b01111110,    
    0b00111100,    
    0b00011000,    
    0b00000000
};

// ET6.2 : état de l'enregistrement, affiché en bas de l'écran 1
enum EtatEnregistrement {
    AUCUN_ENREGISTREMENT,
    ENREGISTREMENT_EN_COURS,
    ENREGISTREMENT_TERMINE
};

// Prototypes des fonctions
void choisirEcran(uint8_t adresse); 
void afficherAccueil(uint8_t indexLangue);
void definirLangue(uint8_t l);
void afficherDateEcran(const char* dateStr);
void afficherMenuSelection(uint8_t indexSelection); 
void afficherMenuAction(uint8_t indexAction); 
void afficherTitreHistorique(uint8_t personneId); 
void afficherListeHistorique(uint8_t personneId, uint8_t indexHistoriqueSelection); 
void afficherDetailMesure(Mesure m, uint8_t personneId);
void afficherEcran1(uint8_t heure, uint8_t minute, uint8_t seconde, float bpm, EtatEnregistrement etat, uint8_t nbMesures);
void afficherEcran2(int valeurPPG); 
void afficherMessageQuitter(); 
void afficherAuRevoir(); 
void eteindreEcrans();

#endif