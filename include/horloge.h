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

extern DateHeure maintenant;   // date et heure courantes, mises a jour par horlogeMettreAJour()

void horlogeInit();
void horlogeMettreAJour();
void horlogeRegler(byte h, byte mi, byte s);
void horlogeReglerDate(byte j, byte mo, byte a);
void horlogeDateTexte(char* buf);

#endif
