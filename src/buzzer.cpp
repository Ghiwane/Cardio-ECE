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
                if(buzzerActif) buzzerActif = false; // on change l'etat du buzzer
                else buzzerActif = true;
            }
            
        etatPrecedent = etatBouton; // pour savoir quand on n'appuie plus sur le bouton
        dernierChangement = millis(); // sauvegarde du moment d'appuie
        }
    }
}

void gererBuzzer(float bpm){
    if (!buzzerActif || bpm <= 0) // si le buzzer est désactivé ou valeur negative du bpm on fait rien
        return;

    int tonalite; // variable pour avoir differentes tonalités selon le bpm
    if(bpm < SEUIL_BPM_BAS) tonalite = 200; // tonalité grave
    else if (bpm > SEUIL_BPM_HAUT) tonalite = 800; // tonalité aigu
    else tonalite = 500; // tonalité normale
    
    tone(PIN_BUZZER, tonalite, 100); // on fait sonner le buzzer
}