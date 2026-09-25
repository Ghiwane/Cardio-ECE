#ifndef CAPTEUR_H
#define CAPTEUR_H

#include <Arduino.h>

void capteurInit();
int lireCapteurBrut();
float calculerBPM(int valeurBrute);

#endif
