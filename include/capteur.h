#ifndef CAPTEUR_H
#define CAPTEUR_H

#include <Arduino.h>

// Variables lisibles par le reste du projet (definies dans capteur.cpp)
extern float ppgBpm;         // 0 si la mesure n'est pas fiable (-> tiret a l'ecran)
extern bool  ppgBpmValide;
extern float ppgNorm;        // signal recentre et normalise (environ -1..1)

int ppgTraiter();            // a appeler a chaque tour de loop() : renvoie 0..1023 pour l'ecran PPG

#endif
