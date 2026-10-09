#ifndef MEMOIRE_H
#define MEMOIRE_H

#include <Arduino.h>

struct Mesure {
    uint8_t valeur;
    uint8_t heure;
    uint8_t minute;
    uint8_t jour;
    uint8_t mois;
};

void ajouterMesure(uint8_t personneId, uint8_t valeur, uint8_t heure, uint8_t minute, uint8_t jour, uint8_t mois);
void recupererHistorique(uint8_t personneId, Mesure tableauDest[3]);
void effacerHistorique(uint8_t personneId);

#endif
