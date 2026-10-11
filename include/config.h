#ifndef CONFIG_H
#define CONFIG_H

//capteur cardiaque (FS1)
#define PIN_CAPTEUR       A0   // sortie analogique du WPSE340

//horloge DS1302 (FS2)
#define PIN_RTC_CLK       A1
#define PIN_RTC_DAT       A2
#define PIN_RTC_RST       A3   // CE du DS1302
#define FORCER_REGLAGE_HORLOGE 0 // 1 = remet la date/heure de compilation au démarrage (téléverser, puis remettre 0 et re-téléverser)

//LED état de santé 
#define PIN_LED_ROUGE     2
#define PIN_LED_VERTE     3
#define PIN_LED_JAUNE     4

//buzzer (FS4)
#define PIN_BUZZER        5
#define PIN_BOUTON_BEEP   6
#define DUREE_MIN_REBOND 50 // durée du rebond du bouton en ms

//écrans OLED : bus I2C sur A4 (SDA) et A5 (SCL)

//bouton / encodeur 
#define PIN_BOUTON_MENU         7   // valider dans les menus
#define PIN_ENCODEUR_CLK        8   // doit rester sur D8 : interruption PCINT0 (voir main.cpp)
#define PIN_ENCODEUR_DT         9

//enregistrement des mesures 
#define PIN_BOUTON_ENREGISTRER  10  // lance l'enregistrement d'une mesure
#define NB_MESURES_MOYENNE      10  //un enregistrement est la moyenne de 10 mesures consecutives

//Seuils fréquence cardiaque en bpm
#define SEUIL_BPM_BAS     60
#define SEUIL_BPM_HAUT    100
#define BPM_MIN 40   // hors de [BPM_MIN ; BPM_MAX] : valeur aberrante
#define BPM_MAX 200


#endif
