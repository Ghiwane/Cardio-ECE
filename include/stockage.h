#ifndef STOCKAGE_H
#define STOCKAGE_H

#include "horloge.h"

struct Enregistrement {
    float bpm;
    DateHeure date;
};

void stockageInit();
void enregistrerMesure(float bpm, DateHeure date);
Enregistrement lireMesure(int index);
int nombreMesures();
void effacerMesures();

#endif
