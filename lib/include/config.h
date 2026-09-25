#ifndef CONFIG_H
#define CONFIG_H

//capteur cardiaque (FS1) 
#define PIN_CAPTEUR       A0   // sortie analogique du WPSE340

//LED état de santé (FS3)
#define PIN_LED_ROUGE     2
#define PIN_LED_VERTE     3
#define PIN_LED_JAUNE     4

//buzzer (FS4)
#define PIN_BUZZER        5
#define PIN_BOUTON_BEEP   6

//bouton / encodeur (FS6)
#define PIN_BOUTON_ENREGISTRER  7
#define PIN_ENCODEUR_CLK        8
#define PIN_ENCODEUR_DT         9
#define PIN_ENCODEUR_SW         10

//Seuils fréquence cardiaque (FS3/FS4) en bpm
#define SEUIL_BPM_BAS     60
#define SEUIL_BPM_HAUT    100


#endif
