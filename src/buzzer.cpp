#include "buzzer.h"
#include "config.h"
#include <Arduino.h>

static bool buzzerActif = true; // flag pour savoir si le buzzer est actif

void buzzerInit(){
    pinMode(PIN_BUZZER, OUTPUT); // initialisation du buzzer
    pinMode(PIN_BOUTON_BEEP, INPUT_PULLUP); // initialisation du bouton poussoir en pull-up interne
}

void etatBuzzer(int etatBouton){
    static int etatPrecedent = HIGH; // c'est pour verifier qu'on appuie bien sur le bouton
    static unsigned long dernierChangement = 0;

    if (etatBouton != etatPrecedent){
        if(millis() - dernierChangement > DUREE_MIN_REBOND){// détection d'un seul appui sans interruption
            if(etatBouton == LOW){
                buzzerActif = !buzzerActif; // on change l'etat du buzzer
                if (!buzzerActif) noTone(PIN_BUZZER); // coupe tout de suite le beep en cours
            }

        etatPrecedent = etatBouton; // pour savoir quand on n'appuie plus sur le bouton
        dernierChangement = millis(); // sauvegarde du moment d'appuie
        }
    }
}

void gererBuzzer(float bpm){
    static unsigned long dernierBeep = 0;

    if (!buzzerActif || bpm < BPM_MIN || bpm > BPM_MAX) // si le buzzer est désactivé ou valeur aberrante du bpm on fait rien
        return;

    if (millis() - dernierBeep < 60000.0 / bpm) // un beep par battement : periode = 60000 / bpm (ms)
        return;
    dernierBeep = millis();

    int tonalite; // variable pour avoir differentes tonalités selon le bpm
    if(bpm < SEUIL_BPM_BAS) tonalite = 200; // tonalité grave
    else if (bpm > SEUIL_BPM_HAUT) tonalite = 800; // tonalité aigu
    else tonalite = 500; // tonalité normale

    tone(PIN_BUZZER, tonalite, 100); // on fait sonner le buzzer
}
