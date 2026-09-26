#ifndef BUZZER_H
#define BUZZER_H

void buzzerInit(); // initialisation du buzzer
void gererBuzzer(float bpm); // gestion des tonnalités du buzzer
void etatBuzzer(int etatBouton); // detection d'appuie du bouton pour stopper ou reprendre les beep

#endif
