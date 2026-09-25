#ifndef HORLOGE_H
#define HORLOGE_H

#include <Arduino.h>

struct DateHeure {
    uint8_t jour;
    uint8_t mois;
    uint16_t annee;
    uint8_t heure;
    uint8_t minute;
    uint8_t seconde;
};

void horlogeInit();
DateHeure lireHeure();

#endif
