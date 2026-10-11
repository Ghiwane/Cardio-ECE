#include <EEPROM.h>
#include "memoire.h"

// Taille totale occupée : 4 * 3 * 5 = 60 octets (sur 1024)
#define NB_PERSONNES 4
#define NB_MESURES   3

// adresse en EEPROM de la mesure i de la personne
int adresseMesure(uint8_t personneId, uint8_t i) {
    return (personneId * NB_MESURES + i) * sizeof(Mesure);
}

void recupererHistorique(uint8_t personneId, Mesure tableauDest[3]) {
    if (personneId >= NB_PERSONNES) return;

    for (int i = 0; i < NB_MESURES; i++) {
        EEPROM.get(adresseMesure(personneId, i), tableauDest[i]);
    }
}

void ajouterMesure(uint8_t personneId, uint8_t valeur, uint8_t heure, uint8_t minute, uint8_t jour, uint8_t mois) {
    if (personneId >= NB_PERSONNES) return;

    Mesure historique[NB_MESURES];
    recupererHistorique(personneId, historique);

    // la plus ancienne mesure est perdue, la nouvelle prend la place 0
    historique[2] = historique[1];
    historique[1] = historique[0];
    historique[0] = {valeur, heure, minute, jour, mois};

    // put() n'ecrit que les octets qui changent (l'EEPROM s'use a chaque ecriture)
    for (int i = 0; i < NB_MESURES; i++) {
        EEPROM.put(adresseMesure(personneId, i), historique[i]);
    }
}

//efface les 3 mesures de la personne (255 = case vide, comme une EEPROM neuve)
void effacerHistorique(uint8_t personneId) {
    if (personneId >= NB_PERSONNES) return;

    Mesure vide = {255, 255, 255, 255, 255};
    for (int i = 0; i < NB_MESURES; i++) {
        EEPROM.put(adresseMesure(personneId, i), vide);
    }
}
