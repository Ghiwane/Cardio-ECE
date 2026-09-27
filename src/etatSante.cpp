#include "etatSante.h"
#include "config.h"
#include <Arduino.h>

void etatSanteInit(){
    pinMode(PIN_LED_ROUGE, OUTPUT); //initialisation des leds
    pinMode(PIN_LED_VERTE, OUTPUT);
    pinMode(PIN_LED_JAUNE, OUTPUT);

    digitalWrite(PIN_LED_ROUGE, LOW); //on s'assure quelles soient eteintes
    digitalWrite(PIN_LED_VERTE, LOW);
    digitalWrite(PIN_LED_JAUNE, LOW);
}

void afficherEtatSante(float bpm){
    bool valeurRealiste = (bpm >= BPM_MIN) && (bpm <= BPM_MAX); //le bpm n'est pas une valeur aberrante true sinon false

    digitalWrite(PIN_LED_ROUGE, LOW); // on eteint les leds 
    digitalWrite(PIN_LED_VERTE, LOW);
    digitalWrite(PIN_LED_JAUNE, LOW);

    if (!valeurRealiste){
        return; // aucun voyant ne s'allume si le bpm est une valeur aberrante
    }

    //plage de valeurs conforme a la medecine
    if (bpm > SEUIL_BPM_HAUT){
        digitalWrite(PIN_LED_ROUGE, HIGH); // frequence elevee
    } else if (bpm < SEUIL_BPM_BAS){
        digitalWrite(PIN_LED_JAUNE, HIGH); // frequence faible
    } else {
        digitalWrite(PIN_LED_VERTE, HIGH); // frequence normale
    }
}