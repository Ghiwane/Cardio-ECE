#include "horloge.h"
#include "config.h"

//  Registres du DS1302 
const byte REG_SEC     = 0x80;
const byte REG_MIN     = 0x82;
const byte REG_HEURE   = 0x84;
const byte REG_DATE    = 0x86;
const byte REG_MOIS    = 0x88;
const byte REG_ANNEE   = 0x8C;
const byte REG_PROTECT = 0x8E;

//  Date et heure courantes 
DateHeure maintenant = {1, 1, 2000, 0, 0, 0};
unsigned long derniereLecture = 0;
const unsigned long PERIODE_LECTURE_MS = 200;

//  Utilitaires 
byte decVersBcd(byte d) { return ((d / 10) << 4) | (d % 10); }
byte bcdVersDec(byte b) { return ((b >> 4) * 10) + (b & 0x0F); }

//  Communication 3 fils avec le DS1302 
void ecrireOctet(byte b) {
  pinMode(PIN_RTC_DAT, OUTPUT);
  for (int i = 0; i < 8; i++) {
    digitalWrite(PIN_RTC_DAT, (b >> i) & 1);
    digitalWrite(PIN_RTC_CLK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_RTC_CLK, LOW);
    delayMicroseconds(2);
  }
}

byte lireOctet() {
  pinMode(PIN_RTC_DAT, INPUT);
  byte v = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(PIN_RTC_DAT)) v |= (1 << i);
    digitalWrite(PIN_RTC_CLK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_RTC_CLK, LOW);
    delayMicroseconds(2);
  }
  return v;
}

void ecrireRegistre(byte reg, byte val) {
  digitalWrite(PIN_RTC_CLK, LOW);
  digitalWrite(PIN_RTC_RST, HIGH);
  ecrireOctet(reg);
  ecrireOctet(val);
  digitalWrite(PIN_RTC_RST, LOW);
}

byte lireRegistre(byte reg) {
  digitalWrite(PIN_RTC_CLK, LOW);
  digitalWrite(PIN_RTC_RST, HIGH);
  ecrireOctet(reg | 0x01);   // bit0 = 1 -> lecture
  byte v = lireOctet();
  digitalWrite(PIN_RTC_RST, LOW);
  return v;
}

//  Fonctions du module 
void horlogeRegler(byte h, byte mi, byte s) {
  ecrireRegistre(REG_PROTECT, 0x00);          // desactive la protection en ecriture
  ecrireRegistre(REG_SEC,   decVersBcd(s));   // bit7 (CH) = 0 -> l'horloge tourne
  ecrireRegistre(REG_MIN,   decVersBcd(mi));
  ecrireRegistre(REG_HEURE, decVersBcd(h));   // mode 24 h
  ecrireRegistre(REG_PROTECT, 0x80);
  maintenant.heure = h; maintenant.minute = mi; maintenant.seconde = s;
}

void horlogeReglerDate(byte j, byte mo, byte a) {
  ecrireRegistre(REG_PROTECT, 0x00);
  ecrireRegistre(REG_DATE,  decVersBcd(j));
  ecrireRegistre(REG_MOIS,  decVersBcd(mo));
  ecrireRegistre(REG_ANNEE, decVersBcd(a));
  ecrireRegistre(REG_PROTECT, 0x80);
  maintenant.jour = j; maintenant.mois = mo; maintenant.annee = 2000 + a;
}

void horlogeInit() {
  pinMode(PIN_RTC_CLK, OUTPUT);
  pinMode(PIN_RTC_RST, OUTPUT);
  digitalWrite(PIN_RTC_RST, LOW);
  digitalWrite(PIN_RTC_CLK, LOW);

  // Module arrete (CH = 1) ou date incoherente (jamais regle, pile vide) : reglage avec la date
  // et l'heure de compilation. Sinon on n'y touche pas : l'heure est conservee apres une coupure (ET2.3).
  // FORCER_REGLAGE_HORLOGE (config.h) permet de le refaire volontairement.
  byte jourLu = bcdVersDec(lireRegistre(REG_DATE) & 0x3F);
  byte moisLu = bcdVersDec(lireRegistre(REG_MOIS) & 0x1F);
  bool dateIncoherente = (jourLu < 1 || jourLu > 31 || moisLu < 1 || moisLu > 12);
  if (FORCER_REGLAGE_HORLOGE || (lireRegistre(REG_SEC) & 0x80) || dateIncoherente) {
    byte h  = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');
    byte mi = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');
    byte s  = (__TIME__[6] - '0') * 10 + (__TIME__[7] - '0');
    horlogeRegler(h, mi, s);

    // __DATE__ = "Oct  6 2026" (espace devant le jour s'il n'a qu'un chiffre)
    // noms des mois en memoire flash (PROGMEM) pour ne pas occuper la RAM
    static const char moisNoms[] PROGMEM = "JanFebMarAprMayJunJulAugSepOctNovDec";
    byte mo = 1;
    for (int i = 0; i < 12; i++) {
      if (strncmp_P(__DATE__, moisNoms + 3 * i, 3) == 0) mo = i + 1;
    }
    byte j = (__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0');
    byte a = (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');
    horlogeReglerDate(j, mo, a);
  }
  derniereLecture = millis() - PERIODE_LECTURE_MS;   // force une lecture immediate
}

void horlogeMettreAJour() {
  unsigned long now = millis();
  if (now - derniereLecture < PERIODE_LECTURE_MS) return;
  derniereLecture = now;

  byte s1, s2, mi, h, j, mo, a;
  int essais = 0;
  do {   // relit si les secondes ont change pendant la lecture
    s1 = lireRegistre(REG_SEC);
    mi = lireRegistre(REG_MIN);
    h  = lireRegistre(REG_HEURE);
    j  = lireRegistre(REG_DATE);
    mo = lireRegistre(REG_MOIS);
    a  = lireRegistre(REG_ANNEE);
    s2 = lireRegistre(REG_SEC);
    essais++;
  } while (s1 != s2 && essais < 3); // 3 essais max : un module mal branche ne doit pas bloquer la boucle

  maintenant.seconde = bcdVersDec(s1 & 0x7F);
  maintenant.minute  = bcdVersDec(mi & 0x7F);
  maintenant.heure   = bcdVersDec(h  & 0x3F);
  maintenant.jour    = bcdVersDec(j  & 0x3F);
  maintenant.mois    = bcdVersDec(mo & 0x1F);
  maintenant.annee   = 2000 + bcdVersDec(a);
}

void horlogeDateTexte(char* buf) {        // buf : au moins 11 caracteres
  buf[0] = '0' + maintenant.jour / 10; buf[1] = '0' + maintenant.jour % 10; buf[2] = '/';
  buf[3] = '0' + maintenant.mois / 10; buf[4] = '0' + maintenant.mois % 10; buf[5] = '/';
  buf[6] = '2'; buf[7] = '0';
  buf[8] = '0' + (maintenant.annee % 100) / 10; buf[9] = '0' + maintenant.annee % 10; buf[10] = '\0';
}
